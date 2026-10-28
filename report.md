# BombLab 实验报告

宓融
25300120001


##  第一关



`phase_01` 调用 `sscanf` 后以 `cmp $0x1,%eax` 检查返回值，说明口令为一个整数。入口把 `challenge` 从 `rsi` 保存到 `r12`，因为后续调用需要复用参数寄存器。随后调用 `p1_compute`，并比较其返回值与输入整数。

`p1_compute` 的部分关键指令为：

```asm
mov %rdi,%rdx
mov %rdi,%rax
shr $0x20,%rax
mov %rdx,%rsi
mov %eax,%edi
shr $0x10,%rsi
call p1_step1
```

调用 `p1_step1` 时，第一个 32 位参数来自 `challenge >> 32`，第二个来自 `challenge >> 16`。三个小函数的核心指令分别是：

```asm
# p1_step1
lea 0x14e7(%rsi),%eax
xor %edi,%eax

# p1_step0
lea (%rdi,%rsi,4),%eax
xor $0x4890,%eax

# p1_step2
xor $0xe120,%esi
lea (%rsi,%rdi,4),%eax
```


定义：

```c
uint32_t lo  = (uint32_t)challenge;
uint32_t hi  = (uint32_t)(challenge >> 32);
uint32_t mid = (uint32_t)(challenge >> 16);
```

三个函数的调用顺序是 `step1 → step0 → step2`，并不是按函数名中的编号排序。根据调用前重新安排的参数，可得到：

```c
uint32_t s1 = (mid + 0x14e7) ^ hi;
uint32_t s0 = (lo + 4 * hi) ^ 0x4890;
uint32_t answer = 4 * s0 + (s1 ^ 0xe120);
```

其中 `mid` 取原数第 16～47 位，与 `lo`、`hi` 存在重叠。判断参数时必须查看每次 `call` 前寄存器中的值，不能一直把某个寄存器与最初的含义绑定。


非公开 seed `314159` 对应本关 challenge `0xd85dda4696932a2d`，验证输出为 `Phase 1 defused.`。

![第一关：seed 314159 验证通过](<images/屏幕截图 2026-10-27 143048.png
## 第二关：数组与循环递推

`sscanf` 后检查返回值是否等于 6，因此本关需要六个整数。首项检查为：

```asm
mov %r13,%rax
shr $0x2b,%rax
and $0x7f,%eax
cmp %eax,%esi
```

`r13` 保存 `challenge`，`0x2b` 等于 43，所以首项为：

```c
values[0] = (uint32_t)((challenge >> 43) & 0x7f);
```

循环开始前 `ecx = 0x1a = 26`，`edx = 1`。核心指令为：

```asm
mov %r13,%rax
shr %cl,%rax
and $0xf,%eax
add %edx,%eax
xor %esi,%eax
mov -0x40(%rbp,%rdx,4),%esi
lea (%rax,%rax,2),%eax
cmp %eax,%esi
```

循环开始时，`esi` 保存上一个输入整数。每轮先从 `challenge` 中提取四位，加上当前下标，再与上一项异或，最后乘以 3，得到当前项的期望值。索引读址中的比例 4 对应每个整数占 4 字节。

每轮 `rdx` 增加 1、`ecx` 增加 5，因此五轮移位次数依次是 26、31、36、41、46。等价递推为：

```c
for (unsigned i = 1; i < 6; ++i) {
    unsigned shift = 26 + 5 * (i - 1);
    uint32_t part = (uint32_t)((challenge >> shift) & 0xf);
    values[i] = 3 * ((part + i) ^ values[i - 1]);
}
```


下图记录 seed `42` 的第二关通过结果。另以非公开 seed `314159` 验证，challenge 为 `0x049392bee0cdfbbd`，输出 `Phase 2 defused.`；该次记录也包含在第九节的完整流程验证中。

![第二关：seed 42 验证通过](<images/屏幕截图 2026-10-27 161320.png>)

## 第三关

本关需要两个整数。第一个整数必须等于 `challenge & 7`，范围为 0～7。调用分派函数前，另一个参数由下式生成：

```c
uint32_t index = (uint32_t)(challenge & 7);
uint32_t x = (uint32_t)(challenge >> 32)
           ^ (uint32_t)(challenge >> 3);
```

`p3_dispatch` 的关键指令是：

```asm
cmp $0x7,%eax
ja  0x402295
jmp *0x405820(,%rax,8)
```

`ja` 检查无符号下标是否超出 7，间接跳转从 `0x405820 + index * 8` 处读取目标地址。每个表项占 8 字节。表项中的地址采用小端序，例如下标 2 的表项位于 `0x405830`，字节为 `78 22 40 00 00 00 00 00`，对应地址 `0x402278`，再跳转到 `p3_case_0`。

因此，表项顺序不能直接用函数名编号代替。


逐项读取跳转表及其目标函数后，得到：

| index | 实际函数 | 第二个整数的表达式 |
|---|---|---|
| 0 | `p3_case_2` | `(2 * x) ^ 0x181c` |
| 1 | `p3_case_3` | `(x >> 2) + (x ^ 0x7e25)` |
| 2 | `p3_case_0` | `5 * x + 0x4975f` |
| 3 | `p3_case_1` | `3 * x + 0x5b3` |
| 4 | `p3_case_5` | `5 * x + 0x46208` |
| 5 | `p3_case_4` | `(2 * x) ^ 0x4c21` |
| 6 | `p3_case_6` | `5 * x + 0xb803` |
| 7 | `p3_case_7` | `(x >> 2) + (x ^ 0xc153)` |

例如 `lea 0x4975f(%rdi,%rdi,4),%eax` 计算的是 `x + 4*x + 0x4975f`。对于先复制、再移位的分支，需要分别跟踪寄存器：一个保存原始 `x` 用于异或，另一个保存 `x >> 2`，最后相加。

代码用 `switch` 实现分支，输出顺序为 `index`、分支计算结果，每个 `case` 末尾使用 `break`。


本关公开测试为 10/10。非公开 seed `314159` 对应 challenge `0x1cd4d8174d7decfd`，输出 `Phase 3 defused.`。

![第三关：公开测试和 seed 314159 验证](<images/屏幕截图 2026-10-28 122653.png>)

## 第四关


本关需要一个整数。调用 `p4_walk` 时，四个参数依次为：

```c
uint32_t target = (uint32_t)(challenge & 0x7f);
uint32_t key = (uint32_t)((challenge >> 7) ^ (challenge >> 32));
// p4_walk(target, 0, 127, key)
```

目标范围是 0～127，初始搜索区间为 `[0, 127]`。


函数入口执行 `high - low`、逻辑右移一位、加回 `low`，得到中点：

```c
uint32_t mid = low + ((high - low) >> 1);
```

当 `target == mid` 时，跳入不再递归的分支：

```asm
lea 0xc(%rdi),%eax
xor %ecx,%eax
ret
```

由此得到基例返回值 `(mid + 12) ^ key`。若 `target < mid`，`jb` 转入左分支，将上界改为 `mid - 1`；否则将下界改为 `mid + 1`。

左分支递归返回后执行：

```asm
lea (%rax,%rax,2),%eax
xor %ebx,%eax
...
xor $0x5,%eax
```

`ebx` 保存当前层的 `mid + key`。右分支的返回处理相同，但最后异或的常量是 `0xc`。因此完整关系为：

```text
mid = low + ((high - low) >> 1)

target == mid：返回 (mid + 12) ^ key
target <  mid：child = walk(target, low, mid - 1, key)
               返回 (3 * child) ^ (mid + key) ^ 5
target >  mid：child = walk(target, mid + 1, high, key)
               返回 (3 * child) ^ (mid + key) ^ 12
```

例如初始中点为 63，目标为 20 时，下一层区间变为 `[0, 62]`。区间不断缩小直到命中目标，随后逐层处理返回值。只找到目标还不够，口令还包含整个返回路径的计算。


本关公开测试为 10/10。非公开 seed `314159` 对应 challenge `0x6a9e8d40d5354fbc`，输出 `Phase 4 defused.`。

![第四关：公开测试结果及额外 seed 通过结果](<images/屏幕截图 2026-10-28 140548.png>)

## 第五关


本关需要两个整数。`p5_traverse` 的第一个参数是 `challenge`，第二个参数是栈上输出位置的地址。返回值用于检查第一个整数，通过指针写入的值用于检查第二个整数。

节点访问的关键指令为：

```asm
mov (%rdx),%rdx
mov 0x8(%rdx),%esi
add 0xc(%rdx),%eax
```

据此将节点恢复为以下结构：

```c
struct Node {
    const struct Node *next;  // 偏移 0，8 字节
    uint32_t tag;            // 偏移 8，4 字节
    uint32_t value;          // 偏移 12，4 字节
};
```

每个节点占 16 字节。节点数组起始地址为 `0x405ba0`，因此原地址 `addr` 对应的数组下标为 `(addr - 0x405ba0) / 16`。例如 `0x405bd0` 对应下标 3。


从只读数据段按小端序解读节点，得到下表。字段名为分析时自行命名。

| 节点下标 | next 下标 | tag | value |
|---|---|---|---|
| 0 | NULL | `0x0125` | `0x0916` |
| 1 | 7 | `0x0656` | `0x3720` |
| 2 | 6 | `0x026e` | `0x4bab` |
| 3 | 4 | `0x047e` | `0xaead` |
| 4 | 2 | `0x0824` | `0xe004` |
| 5 | 10 | `0x071f` | `0x6cc9` |
| 6 | 8 | `0x094b` | `0x1cd0` |
| 7 | 3 | `0x055a` | `0x1f7a` |
| 8 | 9 | `0x0b6b` | `0x282a` |
| 9 | 11 | `0x0a0e` | `0xb0d6` |
| 10 | 0 | `0x000d` | `0x2f0c` |
| 11 | 5 | `0x034e` | `0x6ad5` |

起始节点表位于 `0x405b60`，八项依次指向节点 `1、7、3、4、2、6、8、9`。重新实现时使用本程序节点数组的地址，不能直接使用炸弹进程中的地址。


从 `challenge` 得到：

```c
uint32_t start = (uint32_t)(challenge & 7);
uint32_t steps = (uint32_t)((challenge >> 3) & 3);
uint32_t acc = (uint32_t)(challenge >> 6)
             ^ (uint32_t)(challenge >> 32) ^ 0x829b;
```

先选择起始节点，立即处理一次，再沿 `next` 移动 `steps` 次，每到一个节点都执行：

```c
acc = (acc * 3 + node->value) ^ node->tag;
```

`steps` 表示移动次数，不是处理节点数，所以总共处理 `steps + 1` 个节点。`steps == 0` 时仍处理起点。最终第一个输出为 `acc`，第二个输出为最后访问节点的 `tag`。


最初节点 4 的 `next` 被错误填写为节点 6，导致 seed `99` 和 `18446744073709551615` 失败，公开测试只有 8/10。重新核对原始字节 `c0 5b 40 00 00 00 00 00` 后，确认其地址为 `0x405bc0`，即节点 2。修正后重新编译，公开测试达到 10/10。

此外，编写遍历循环时曾漏掉循环外对起始节点的处理，在代码检查时补回。该步骤确保零步移动也能得到正确结果。

非公开 seed `314159` 对应 challenge `0xd2b58ef282236824`，输出 `Phase 5 defused.`。

![第五关：修正后的公开测试及 seed 314159 验证](<images/屏幕截图 2026-10-28 174523.png>)

##  第六关


本关需要一个整数。`p6_dispatch` 包含两次间接调用：

```asm
and $0x38,%ebx
and $0x7,%eax
call *0x405e00(,%rax,8)
xor $0x8159,%eax
mov %eax,%edi
call *0x405e00(%rbx)
...
add $0xac49,%eax
```

第一次调用的下标是 `challenge & 7`。第二次调用使用 `challenge & 0x38` 作为字节偏移，其值已经是 8 的倍数，换成下标就是 `(challenge >> 3) & 7`。例如最低六位为 `101011` 时，第一次下标为 3，第二次为 5。

第一次输入为：

```c
uint32_t x = (uint32_t)(challenge >> 6)
           ^ (uint32_t)(challenge >> 32);
```


读取 `0x405e00` 处的八项函数地址后，得到函数顺序：

```text
op1, op3, op4, op1, op5, op0, op2, op3
```

六个函数的计算关系如下，输入 `x` 为 `uint32_t`：

```c
op0(x) = (x + 0x962) ^ (x >> 2);
op1(x) = ((x >> 1) ^ x) + 0x2f94;
op2(x) = (x ^ 0x9b27) + 8 * x;
op3(x) = (x >> 2) | (x << 30);
op4(x) = (5 * x) ^ (4 * x + 0xed2);
op5(x) = ((x << 1) | (x >> 31)) ^ 0x2e9e;
```

其中 `op3` 的 `ror $2,%eax` 是 32 位循环右移两位，移出的最低两位补到最高两位；`op5` 的 `rol $1,%eax` 是循环左移一位。它们不能用普通移位单独代替。

代码通过 `typedef uint32_t (*Operation)(uint32_t)` 定义函数指针类型，再构造八项函数表。整体流程为：

```c
first = operations[first_index](x);
second = operations[second_index](first ^ 0x8159);
answer = second + 0xac49;
```

本关还需要注意运算优先级。例如 `(x >> 1) ^ x + constant` 会先计算加法，不等于先异或后加常量；实现时为中间步骤添加明确的括号。


本关公开测试为 10/10。非公开 seed `314159` 对应 challenge `0xc1a32713817bbecc`，输出 `Phase 6 defused.`。

![第六关：公开测试及 seed 314159 验证](<images/屏幕截图 2026-10-28 195322.png>)

##  Honor


隐藏关先经过 `secret_gate`。它把输入复制到从 `rbp - 0xd0` 开始的局部区域，然后检查 `rbp - 0xb0` 处的四字节数：

```asm
lea -0xd0(%rbp),%r12
...
call secret_copy
cmpl $0x4e45504f,-0xb0(%rbp)
...
lea -0xac(%rbp),%rdi
call secret_phase
```

检查字段相对复制起点的偏移为 `0xd0 - 0xb0 = 32`。常量 `0x4e45504f` 按小端序对应字节 `4f 50 45 4e`，即 ASCII 字符串 `OPEN`。传给 `secret_phase` 的文本起点偏移为 `0xd0 - 0xac = 36`。

因此构造的整行结构为：

```text
偏移 0～31：32 个 A
偏移 32～35：OPEN
偏移 36 起：三个空格分隔的无符号十进制整数
```

`secret_copy` 按字节复制到字符串结束符，没有在 32 字节处停止。因此这段输入能够同时设置后面的检查字段和数字文本。本次构造依据局部对象的布局完成。

`secret_gate` 还用无符号比较检查 `strlen(input) - 36 <= 127`，即输入长度应在 36～163 字节之间。三个 32 位无符号整数各最多十位，加上两个空格与 36 字节前缀，输出最长为 68 字节，满足该范围。


`secret_phase` 要求 `sscanf` 返回 3。本关除 `challenge` 外还使用 `context`，后者由驱动根据前六关口令生成并传给 `solve_secret`。

函数中 `r12` 保存 `challenge`，`r13` 保存 `context`。第一个整数的检查是：

```asm
mov %r13d,%ecx
and $0x3,%ecx
cmp %ecx,-0x2c(%rbp)
```

因此：

```c
uint32_t first = (uint32_t)(context & 3);
```

随后分别将两个 64 位输入的高、低 32 位异或，得到：

```c
uint32_t a = (uint32_t)(challenge >> 32) ^ (uint32_t)challenge;
uint32_t b = (uint32_t)(context >> 32) ^ (uint32_t)context;
```

`secret_mix` 的关键指令为：

```asm
mov %edi,%eax
xor $0xe2ca,%edi
shr $0x5,%eax
xor $0x31cc,%eax
add %edi,%eax
```

它把原始输入分成两路计算，恢复为：

```c
mix(x) = (x ^ 0xe2ca) + ((x >> 5) ^ 0x31cc);
```

结合调用前后的乘法、移位和异或，第二、第三个整数为：

```c
uint32_t extra = (first * 0xe2ca) ^ (uint32_t)(context >> 16);
uint32_t second = secret_mix(a ^ b) ^ extra;
uint32_t third = second ^ (0x31cc + b + first + (a >> 5));
```

其中 `(a >> 5)` 必须单独括起，否则写成 `sum + a >> 5` 会变成整个加法结果右移。全部中间运算使用 32 位无符号类型，与寄存器宽度对应。


使用 `snprintf` 将前缀和三个数写入 `out`。两个相邻的 16 字符 `A` 字符串在 C 中自动拼接，组成准确的 32 字节填充。使用 `PRIu32` 输出无符号十进制整数，并检查 `snprintf` 返回值是否为负或不小于 `cap`。成功时返回 0，驱动负责输出整行。

Honor 公开测试为 10/10；非公开 seed `314159` 的完整流程也输出 `Secret phase defused.`。下图记录隐藏关解除结果，截图本身未显示 seed，seed 与命令记录见下一节。

![六关及 Honor 解除结果](<images/屏幕截图 2026-10-28 202346.png>)

