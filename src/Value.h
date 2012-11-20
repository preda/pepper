#pragma once

#include "common.h"

typedef u64 Value;

// Value tags
enum {
    T_OBJ  = 0,
    T_INT,
    T_REG,   // used during compilation only, to indicate register position
    T_DBL  = 7, // special, related to double format
    T_STR  = 8, // up to STRING+6
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
#define EMPTY_STRING VALUE(T_STR, 0)

#define TAG(v) ((unsigned) ((v) >> 48))

#define IS_INT(v) (TAG(v)==T_INT)
static inline bool IS_NUMBER(Value v) {
    unsigned t = TAG(v);
    return t==T_INT || t==T_DBL || (t&0x7ff0);
}

#define LOW(v) ((unsigned)v)
#define UP(v)  ((unsigned)(v>>32))

#define IS_FALSE(v) (LOW(v)==0 && (UP(v)==(T_INT<<16) || UP(v)==0 || (UP(v)&0x7fffffff)==0x7fffffff))

#define TRUE  VAL_INT(1)
#define FALSE VAL_INT(0)
#define ZERO  VAL_INT(0)

#define IS_REG(v) (TAG(v) == T_REG)
#define IS_DOUBLE_TAG(t) (t==T_DBL || t&0x7ff0)
#define IS_NUMBER_TAG(t) (t==T_INT || IS_DOUBLE_TAG(t)) 
#define IS_DOUBLE(v) IS_DOUBLE_TAG(TAG(v))
#define IS_SHORT_STR(v) (TAG(v) >= T_STR && TAG(v) <= T_STR+6)

#define O_TYPE(v) (((Object *)v)->type)
#define IS_OBJ(v,what) (v && TAG(v)==T_OBJ && O_TYPE(v)==what)
#define IS_PROTO(v) IS_OBJ(v, O_PROTO)
#define IS_ARRAY(v) IS_OBJ(v, O_ARRAY)
#define IS_MAP(v)   IS_OBJ(v, O_MAP)
#define IS_STRING(v) (IS_SHORT_STR(v) || IS_OBJ(v, O_STR))



Value VAL_STRING(const char *s, int len);

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
    u.w.w2 ^= 0xffffffff;
    return u.v;
}

static inline s64 getInteger(Value val) {
    return (((s64)val) << 16) >> 16;
}

static inline double getDouble(Value val) {
    if (TAG(val)==T_INT) {
        return getInteger(val);
    } else {
        ValueUnion u{v: val};
        u.w.w2 ^= 0xffffffff;
        return u.d;
    }
}

#define GET_CSTR(v) (TAG(v)==T_OBJ ? ((String*) v)->s : ((char *) &v))

unsigned hashCode(Value a);
unsigned len(Value a);
bool equals(Value a, Value b);
