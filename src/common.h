#pragma once

typedef unsigned char byte;
typedef unsigned long long u64;
typedef long long s64;

u64 hash64(byte *p, int len, u64 h=0);
u64 hash64(const char *p);
static inline u64 hash64Step(byte b, u64 h) { return (h ^ b) * 0x100000001b3ULL; }

u64 error(int err) __attribute__ ((noreturn));
int catchError();
#define ERR(cond, mes) if (cond) { error(mes); }

enum {
    E_VAR_NAME = 1,
    E_NAME_NOT_FOUND,
    E_TODO,
    E_WRONG_TYPE,
    E_DIV_ZERO,
    E_ASSIGN_TO_CONST,
    E_ASSIGN_RHS,
    E_EXPECTED = 256,    
};
