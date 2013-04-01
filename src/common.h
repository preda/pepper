// Copyright (C) 2012 - 2013 Mihai Preda

#pragma once

typedef unsigned char byte;
typedef unsigned long long u64;
typedef long long s64;
typedef u64 Value;

u64 hash64(byte *p, int len, u64 h=0);
u64 hash64(const char *p);
static inline u64 hash64Step(byte b, u64 h) { return (h ^ b) * 0x100000001b3ULL; }

u64 error(const char *file, int line, int err) __attribute__ ((noreturn));
int catchError();
#define ERR(cond, mes) if (cond) { error(__FILE__, __LINE__, mes); }
#define ERROR(mes) error(__FILE__, __LINE__, mes)

#define min(a, b) ((a)<(b)?(a):(b))
#define max(a, b) ((a)<(b)?(b):(a))
#define clamp(x, a, b) ((x)<=(a)?(a):((x)>=(b)?(b):(x)))
#define ASIZE(x) (sizeof(x)/sizeof(x[0]))

enum {
#define _(x) E_##x
#include "error.inc"
#undef _
};
