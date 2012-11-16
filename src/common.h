#pragma once

typedef unsigned char byte;
typedef unsigned long long u64;
typedef long long s64;

u64 hash64(byte *p, int len, u64 h=0);
u64 hash64(const char *p);
static inline u64 hash64Step(byte b, u64 h) { return (h ^ b) * 0x100000001b3ULL; }

#define ERR(cond, mes) if (cond) { error(mes); }
