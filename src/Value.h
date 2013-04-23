// Copyright (C) 2012 - 2013 Mihai Preda

#pragma once

#include "common.h"

typedef u64 Value;
class Object;
class VM;
class GC;

// Value tags
enum {
    T_NIL = -1,
    T_OBJ = -2,
    T_REG = -3, // used during compilation only, to indicate register position
    T_CF  = -4, // plain C function with no data
    T_CP  = -5, // C void pointer

    T_STR0 = -14,
    T_STR1,
    T_STR2,
    T_STR3,
    T_STR4,
    T_STR5,
};

// Object types
enum {
    O_ARRAY = 1,
    O_MAP,
    O_FUNC,
    O_CFUNC,
    O_PROTO,
    O_STR,
};

typedef Value (*tfunc)(VM *vm, int op, void *data, Value *stack, int nCallArg);

union ValueUnion {
    double dbl;
    Value v;
    void *ptr;
    struct {
        int w1;
        unsigned w2;
    };
    struct {
        char chars[6];
        short tag;
    };
};

inline Value VALUE_(short tag, int w) {
    ValueUnion u{tag:tag};
    u.w1 = w;
    return u.v;
}

inline Value VAL_REG(int i) {
    return VALUE_(T_REG, i);
}

inline Value VAL_NUM(double dbl) { return ValueUnion{dbl: dbl}.v; }
inline double GET_NUM(Value val) { return ValueUnion{v: val}.dbl; }

inline Value VAL_PTR(int tag, void *ptr) {
    ValueUnion u{ptr:ptr};
    u.tag = tag;
    return u.v;
}

inline void *GET_PTR(Value v) {
    if (sizeof(void *) == 4) {
        return (void *) v;
    } else {
        ValueUnion u{v:v};
        u.tag = 0;
        return u.ptr;
    }
}

inline Value VAL_OBJ(void *obj) { return VAL_PTR(T_OBJ, obj); }
inline Value VAL_CF(tfunc f)    { return VAL_PTR(T_CF, (void *) f); }
inline Value VAL_CP(void *p)    { return VAL_PTR(T_CP, p); }
inline Object *GET_OBJ(Value v) { return (Object *) GET_PTR(v); }
inline tfunc   GET_CF(Value v)  { return (tfunc) GET_PTR(v); }
inline void *GET_CP(Value v)    { return GET_PTR(v); }

#define STRING(v) ((String *) GET_OBJ(v))
#define ARRAY(v) ((Array *) GET_OBJ(v))
#define MAP(v) ((Map *) GET_OBJ(v))
#define PROTO(v) ((Proto *) GET_OBJ(v))

inline int TAG(Value v) { return ValueUnion{v:v}.tag; }
// #define TAG(v) ((unsigned) ((v) >> 48))

inline Value VAL_TAG(short tag) { return ValueUnion{tag:tag}.v; }
#define VNIL VAL_TAG(T_NIL)
#define IS_NIL(v) (TAG(v) == T_NIL)
#define IS_REG(v) (TAG(v) == T_REG)
#define IS_OBJ(v) (TAG(v) == T_OBJ)
#define IS_CF(v)  (TAG(v) == T_CF)
#define IS_CP(v)  (TAG(v) == T_CP)
inline bool IS_NUM_TAG(int t) { return (unsigned)t <= (unsigned)-16 || t == -8; }
#define IS_NUM(v) IS_NUM_TAG(TAG(v))

//#define EMPTY_STRING ((u64)T_STR0 << 48)
#define EMPTY_STRING VAL_TAG(T_STR0)
#define IS_EMPTY_STRING  (TAG(v) == T_STR0)
#define IS_SHORT_STR(v)  (T_STR0 <= TAG(v) && TAG(v) <= T_STR5)
inline bool IS_SHORT_STR_TAG(int t) { return t >= T_STR0 && t <= T_STR5; }
#define SHORT_STR_LEN(v) (TAG(v) - T_STR0)
// #define VAL_SHORT_STR(c, len) ValueUnion{tag:(T_STR0+len)}.v

#define IS_FALSE(v) (v == ZERO || TAG(v) == T_NIL)
#define IS_TRUE(a) (!IS_REG(a) && !IS_FALSE(a))

#define ZERO  VAL_NUM(0)
#define ONE   VAL_NUM(1)
#define FALSE ZERO
#define TRUE  ONE

#define O_TYPE(v) (GET_OBJ(v)->type())
#define IS_O_TYPE(v,what) (IS_OBJ(v) && O_TYPE(v)==what)
#define IS_PROTO(v) IS_O_TYPE(v, O_PROTO)
#define IS_FUNC(v) IS_O_TYPE(v, O_FUNC)
#define IS_CFUNC(v) (IS_CF(v) || IS_O_TYPE(v, O_CFUNC))
#define IS_ARRAY(v) IS_O_TYPE(v, O_ARRAY)
#define IS_MAP(v)   IS_O_TYPE(v, O_MAP)
#define IS_STRING(v) (IS_SHORT_STR(v) || IS_O_TYPE(v, O_STR))

#define GET_CSTR(v) (IS_OBJ(v) ? ((String*)GET_OBJ(v))->s : (char*)&v)

unsigned hashCode(Value a);
unsigned len(Value a);

bool objEquals(Object *a, Object *b);
inline bool equals(Value a, Value b) {
    return a == b || (IS_OBJ(a) && IS_OBJ(b) && objEquals(GET_OBJ(a), GET_OBJ(b)));
}

bool lessThan(Value a, Value b);
