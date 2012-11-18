#pragma once

#include "common.h"

typedef u64 Value;

// Value tags
enum {
    OBJECT = 0,
    // ARRAY indicates empty array
    // MAP   indicates empty map
    INTEGER  = 3,
    REGISTER, // flag used during compilation only
    
    DOUBLE = 7,
    STRING = 8,
    // future T_FLAG_ARR1 = 0x8000,
};

// Object types
enum {
    ARRAY = 1,
    MAP,
    FUNC,
    CFUNC,
    PROTO,
    // plus STRING
    // future INTEGER
};

#define VALUE(tag,x) ((((u64)(tag)) << 48) | (x))
#define VAL_OBJ(obj) ((Value)(obj))
#define VAL_INT(i)   VALUE(INTEGER,  (i) & 0xffffffffffffLL)
#define VAL_REG(i)   VALUE(REGISTER, (i) & 0xffffffffffffLL)

#define NIL VAL_OBJ(0)

#define EMPTY_STRING VALUE(STRING, 0)
#define EMPTY_ARRAY  VALUE(ARRAY, 0)
#define EMPTY_MAP    VALUE(MAP, 0)

#define TAG(v) ((unsigned) ((v) >> 48))

#define IS_INTEGER(v) (TAG(v)==INTEGER)
static inline bool IS_NUMBER(Value v) {
    unsigned t = TAG(v);
    return t==INTEGER || t==DOUBLE || (t&0x7ff0);
}

#define LOW(v) ((unsigned)v)
#define UP(v) ((unsigned)(v>>32))

#define IS_FALSE(v) (LOW(v)==0 && (UP(v)==(INTEGER<<16) || (UP(v)&0x7fffffff)==0x7fffffff))

#define TRUE  VAL_INT(1)
#define FALSE VAL_INT(0)
#define ZERO  VAL_INT(0)

#define IS_REGISTER(v) (TAG(v) == REGISTER)
#define IS_DOUBLE_TAG(t) (t==DOUBLE || t&0x7ff0)
#define IS_NUMBER_TAG(t) (t==INTEGER || IS_DOUBLE_TAG(t)) 
#define IS_DOUBLE(v) IS_DOUBLE_TAG(TAG(v))

// #define IS_NUMBER(v) (IS_INTEGER(v) || IS_DOUBLE(v))
#define ISOBJ(v,what) (TAG(v)==what || (v && TAG(v)==OBJECT && ((Object*)v)->type==what))
#define IS_ARRAY(v) ISOBJ(v, ARRAY)
#define IS_MAP(v) ISOBJ(v, MAP)
#define IS_STRING(v) ((TAG(v)>=STRING && TAG(v)<=STRING+6) || ISOBJ(v, STRING))

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
    if (TAG(val)==INTEGER) {
        return getInteger(val);
    } else {
        ValueUnion u{v: val};
        u.w.w2 ^= 0xffffffff;
        return u.d;
    }
}

#define GET_CSTR(v) (TAG(v)==OBJECT ? ((String*) v)->s : ((char *) &v))

unsigned hashCode(Value a);
unsigned len(Value a);
void printValue(Value a);
