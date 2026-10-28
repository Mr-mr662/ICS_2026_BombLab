#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    uint32_t lo  = (uint32_t)challenge;
    uint32_t hi  = (uint32_t)(challenge >> 32);
    uint32_t mid = (uint32_t)(challenge >> 16);

    uint32_t s1 = (mid + 0x14e7) ^ hi;
    uint32_t s0 = (lo + 4 * hi) ^ 0x4890;
    uint32_t answer = 4 * s0 + (s1 ^ 0xe120);

    values[0]=answer;
    count=1;

    return write_answer(out, cap, values, count);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    values[0] = (uint32_t)((challenge>>43)&0x7f);
    for (unsigned i = 1; i < 6; ++i) {
        unsigned shift = 26 + 5 * (i - 1);
        uint32_t part = (uint32_t)((challenge >> shift) & 0xf);
	uint32_t temp = part + i;
	temp = temp^values[i-1];
	values[i] = 3*temp;
    }
    count =6;
    return write_answer(out, cap, values, count);
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    uint32_t index = (uint32_t)(challenge & 7);
    uint32_t x = (uint32_t)(challenge >> 32)
           ^ (uint32_t)(challenge >> 3);
    uint32_t result = 0;

    switch (index) {
        case 0:
            result = (2 * x) ^ 0x181c;
            break;
	case 1:
	    result = (x >> 2) + (x ^ 0x7e25);
	    break;
	case 2:
	    result = 5*x + 0x4975f;
	    break;
	case 3:
	    result = 0x5b3 + 3*x;
	    break;
	case 4:
	    result = 0x46208 + 5*x;
	    break;
	case 5:
	    result = (2*x)^0x4c21;
	    break;
	case 6:
	    result = 0xb803 + 5*x;
	    break;
	case 7:
	    result = (x>>2) + (x^0xc153);
	    break;
    }
    values[0] = index;
    values[1] = result;
    count = 2;
    return write_answer(out, cap, values, count);
}

static uint32_t walk(uint32_t target, uint32_t low,
                     uint32_t high, uint32_t key) {
    uint32_t mid = low + ((high - low) >> 1);

    if (target == mid) {
        return (12+mid) ^ key;
    }

    if (target < mid) {
        uint32_t child = walk(target, low, mid - 1, key);
        return ((child*3)^(mid+key))^5;
    } else {
        uint32_t child = walk(target, mid + 1, high, key);
        return ((child*3)^(mid+key))^12;
    }
}
int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    uint32_t target = (uint32_t)(challenge & 0x7f);
    uint32_t key = (uint32_t)((challenge >> 7) ^ (challenge >> 32));

    values[0] = walk(target, 0, 127, key);
    count = 1;
    return write_answer(out, cap, values, count);
}

struct Node {
    const struct Node *next;
    uint32_t tag;
    uint32_t value;
};
static const struct Node nodes[12] = {
	{ NULL,	0x0125,0x0916},
	{ &nodes[7],0x0656,0x3720},
	{ &nodes[6],0x026e,0x4bab},
	{ &nodes[4],0x047e,0xaead},
	{ &nodes[2],0x0824,0xe004},
	{ &nodes[10],0x071f,0x6cc9},
	{ &nodes[8],0x094b,0x1cd0},
	{ &nodes[3],0x055a,0x1f7a},
	{ &nodes[9],0x0b6b,0x282a},
	{ &nodes[11],0x0a0e,0xb0d6},
	{ &nodes[0],0x000d,0x2f0c},
	{ &nodes[5],0x034e,0x6ad5}
};

static const struct Node *const starts[8] = {
    &nodes[1],
    &nodes[7],
    &nodes[3],
    &nodes[4],
    &nodes[2],
    &nodes[6],
    &nodes[8],
    &nodes[9]
};
int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    
    uint32_t start = (uint32_t)(challenge & 7);
    uint32_t steps = (uint32_t)((challenge >> 3) & 3);

    uint32_t acc = (uint32_t)(challenge >> 6)
             ^ (uint32_t)(challenge >> 32)
             ^ 0x829b;

    const struct Node *node = starts[start];
    acc = (acc*3 + node->value)^node->tag;
    for(uint32_t i=0;i<steps;i++){
	    node = node->next;
	    acc = (acc*3 + node->value)^node->tag;
    }
    values[0] = acc;
    values[1] = node->tag;
    count = 2;
    return write_answer(out, cap, values, count);
}

static uint32_t op0(uint32_t x){
	return (x+0x962)^(x>>2);
}
static uint32_t op1(uint32_t x){
	return ((x>>1)^x)+0x2f94;
}
static uint32_t op2(uint32_t x){
	return (x^0x9b27)+x*8;
}
static uint32_t op3(uint32_t x){
	return (x>>2)|(x<<30);
}
static uint32_t op4(uint32_t x){
	return (x*5)^(4*x+0xed2);
}
static uint32_t op5(uint32_t x){
	return ((x<<1)|(x>>31))^0x2e9e;
}

typedef uint32_t (*Operation)(uint32_t);
static const Operation operations[8] = {
	op1,op3,op4,op1,op5,op0,op2,op3
};

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    uint32_t  first_index = (uint32_t)(challenge&7);
    uint32_t  second_index = (uint32_t)((challenge>>3)&7);

    uint32_t x = (uint32_t)(challenge >> 6)^(uint32_t)(challenge>>32);
    uint32_t first = operations[first_index](x);
    uint32_t second = operations[second_index](first^0x8159);
    values[0] = second + 0xac49;
    count = 1;
    return write_answer(out, cap, values, count);
}


static uint32_t secret_mix(uint32_t x) {
    return (x ^ 0xe2ca) + ((x >> 5) ^ 0x31cc);
}

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t first = (uint32_t)(context & 3);
    uint32_t a = (uint32_t)(challenge >> 32) ^ (uint32_t)challenge;
    uint32_t b = (uint32_t)(context >> 32) ^ (uint32_t)context;
    uint32_t extra = (first * 0xe2ca) ^ (uint32_t)(context >> 16);
    uint32_t second = secret_mix(a ^ b) ^ extra;
    uint32_t third = second ^ (0x31cc + b + first + (a >> 5));
    int size = snprintf(out, cap,
    "AAAAAAAAAAAAAAAA"
    "AAAAAAAAAAAAAAAA"
    "OPEN%" PRIu32 " %" PRIu32 " %" PRIu32,
    first, second, third);

    return size < 0 || (size_t)size >= cap;
}
