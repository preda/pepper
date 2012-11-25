#pragma once

#include "common.h"

typedef u64 Value;

// Value tags
enum {
    T_OBJ  = 0,
    T_INT,
    T_REG,   // used during compilation only, to indicate register position
    T_STR6 = 0x8000,
    T_STR5, T_STR4, T_STR3, T_STR2, T_STR1, T_STR0,
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

#define VALUE(tag,x) ((((u64)(tag)) << 48) | (x))
#define VAL_OBJ(obj) ((Value)(obj))
#define VAL_INT(i)   VALUE(T_INT,  (i) & 0xffffffffffffLL)
#define VAL_REG(i)   VALUE(T_REG, (i) & 0xffffffffffffLL)

#define NIL VAL_OBJ(0)
#define EMPTY_STRING VALUE(T_STR0, 0)

#define TAG(v) ((unsigned) ((v) >> 48))

#define LOW(v) ((unsigned)v)
#define UP(v)  ((unsigned)(v>>32))

//is false: 0, NIL, +/-0.0
#define IS_FALSE(v) (LOW(v)==0 && (UP(v)==(T_INT<<16) || UP(v)==0 || (UP(v)&0x7fffffff)==0x7fffffff))

#define TRUE  VAL_INT(1)
#define FALSE VAL_INT(0)
#define ZERO  VAL_INT(0)

#define IS_INT(v) (TAG(v) == T_INT)
#define IS_DOUBLE_TAG(t) (t&0x7ff0 || t==7 || t==0xf)
#define IS_NUMBER_TAG(t) (t==T_INT || IS_DOUBLE_TAG(t)) 
#define IS_DOUBLE(v) IS_DOUBLE_TAG(TAG(v))
static inline bool IS_NUMBER(Value v) { unsigned t = TAG(v); return IS_NUMBER_TAG(t); }

// REG values used during compilation only
#define IS_REG(v) (TAG(v) == T_REG)
#define FLAG_DONT_PATCH (1ull << 32)

#define IS_SHORT_STR(v) (TAG(v) >= T_STR6 && TAG(v) <= T_STR0)
#define SHORT_STR_LEN(v) (T_STR0 - TAG(v))

#define O_TYPE(v) (((Object *)v)->type)
#define IS_OBJ(v) (v && TAG(v) == T_OBJ)
#define IS_O_TYPE(v,what) (IS_OBJ(v) && O_TYPE(v)==what)
#define IS_PROTO(v) IS_O_TYPE(v, O_PROTO)
#define IS_ARRAY(v) IS_O_TYPE(v, O_ARRAY)
#define IS_MAP(v)   IS_O_TYPE(v, O_MAP)
#define IS_STRING(v) (IS_SHORT_STR(v) || IS_O_TYPE(v, O_STR))

union ValueUnion {
    double d;
    Value v;
    struct {
        unsigned w1;
        unsigned w2;
    } w;
};

static inline Value VAL_DOUBLE(double dbl) {
    ValueUnion u{d: dbl};
    u.w.w2 = ~u.w.w2;
    return u.v;
}

static inline s64 getInteger(Value val) {
    return ((s64) val << 16) >> 16;
}

static inline double getDouble(Value val) {
    if (IS_INT(val)) {
        return getInteger(val);
    } else {
        ValueUnion u{v: val};
        u.w.w2 = ~u.w.w2;
        return u.d;
    }
}

#define GET_CSTR(v) (TAG(v)==T_OBJ ? ((String*) v)->s : (char*)&v)

unsigned hashCode(Value a);
unsigned len(Value a);
bool equals(Value a, Value b);
bool lessThan(Value a, Value b);
// bool lessEqual(Value a, Value b);
