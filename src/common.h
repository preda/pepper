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

enum {
    E_VAR_NAME = 1,
    E_NAME_NOT_FOUND,
    E_TODO,
    E_WRONG_TYPE,
    E_DIV_ZERO,
    E_ASSIGN_TO_CONST,
    E_ASSIGN_RHS,
    E_SYNTAX,
    E_OBJECT_TYPE,
    E_SET_NEGATIVE_INDEX,
    E_INDEX_NOT_NUMBER,
    E_NOT_INDEXABLE,
    E_STRING_WRITE,
    E_LEN_NOT_COLLECTION,
    E_ADD_NOT_COLLECTION,
    E_STR_ADD_TYPE,

    E_OPEN_STRING,

    E_FFI_TYPE_MISMATCH,
    E_FFI_VARARG,
    E_FFI_INVALID_SIGNATURE,
    E_FFI_N_ARGS,
    E_FFI_CIF,

    E_CALL_NIL,
    E_CALL_NOT_FUNC,

    E_FOR_NAME,
    E_FOR_NOT_NUMBER,

    E_EXPECTED = 256,
};
