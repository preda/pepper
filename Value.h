#pragma once

#include "Object.h"
#include "String.h"

typedef unsigned long long Value;

enum {
    OBJECT = 0,
    INTEGER,
    ARRAY,
    MAP,
    FUNC,
    CFUNC,
    DOUBLE = 7,
    STRING = 8,
    // T_FLAG_ARR1 = 0x8000,
};

#define VALUE(tag,x) (((unsigned long long)tag << 48) | x)
#define VAL_OBJ(obj) ((Value) obj)
#define VAL_INT(i)   VALUE(INTEGER, (i) & 0xffffffffffffLL)

#define NIL VAL_OBJ(0)
#define ERR VAL_OBJ(1)
#define UNK VAL_OBJ(2)

#define ARRAY0 VALUE(ARRAY, 0)
#define MAP0 VALUE(MAP, 0)
#define STRING0 VALUE(STRING, 0)

#define TAG(v) ((unsigned) ((v) >> 48))

#define IS_INTEGER(v) (TAG(v)==INTEGER)
static inline bool IS_NUMBER(Value v) {
    unsigned t = TAG(v);
    return t==INTEGER || t==DOUBLE || (t&0x7ff0);
}

#define IS_DOUBLE_TAG(t) (t==DOUBLE || t&0x7ff0)
#define IS_NUMBER_TAG(t) (t==INTEGER || IS_DOUBLE_TAG(t)) 
#define IS_DOUBLE(v) IS_DOUBLE_TAG(TAG(v))

// #define IS_NUMBER(v) (IS_INTEGER(v) || IS_DOUBLE(v))
#define ISOBJ(v,what) (TAG(v)==what || (v && TAG(v)==OBJECT && ((Object*)v)->type==what))
#define IS_ARRAY(v) ISOBJ(v, ARRAY)
#define IS_MAP(v) ISOBJ(v, MAP)
#define IS_STRING(v) ((TAG(v)>=STRING && TAG(v)<=STRING+6) || ISOBJ(v, STRING))

Value VAL_STRING(const char *s) {
    
}

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

static inline long long getInteger(Value val) {
    return (((long long)val) << 16) >> 16;
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

/*
static inline char *getCstr(Value val) {
    int tag = tag(val);
    if (tag >= STRING && tag <= STRING+6) {
        return 
    }
}
*/

static unsigned hashCode(Value a) {
    int tag = TAG(a);
    switch (tag) {
    case OBJECT:  return a ? ((Object *)a)->hashCode() : 0;
    case INTEGER: return (unsigned) a * FNV;
    case ARRAY: case MAP: case STRING: return 0;
    default: // STR1-6 or double
        if (tag > STRING && tag <= STRING+6) {
            Value b = a;
            return String::hashCode((char *)&b, tag-STRING);
        } else {
            return (((unsigned) a) ^ ((unsigned) (a >> 32))) * FNV;
        }
    }
}

Value stringConcat(Value a, Value b);
// Value stringConcat(Value a, char *pb, int sb);
// void numberToString(Value a, char *out, int outSize);

/*
union Value {
    double d;
    Object *ptr;
    int intVal;
    struct {
        unsigned dummy1;
        unsigned short dummy2;
        unsigned short tag;
    } t;
    struct {
        unsigned w1;
        unsigned w2;
    }w;
    long long bytes;

    enum Tag {
        OBJ  = 0,
        INT  = 1,
        ARR0 = 2,
        MAP0 = 3,
        STR0 = 8,
        ARR1 = 0x8000,
    };

    bool operator==(Value other) {
        if (bytes == other.bytes) {
            return true;
        }

        if (t.tag != OBJ || other.t.tag != OBJ) {
            return false;
        }

        return ptr->type == STRING && other.ptr->type == STRING &&
        ((String *) ptr)->equals((String *) other.ptr);        
    }

    inline void mark() {
        if (t.tag == OBJ) {
            GC::mark(ptr);
        }
    }

    unsigned hashCode() {
        unsigned tag = t.tag & 0x7fff;
        switch (tag) {
        case OBJ:
            return ptr ? ptr->hashCode() : 0;
        case INT:  
            return (unsigned) (intVal * FNV);
        case ARR0: case MAP0:
            return 0;
        default: // STR0 - STR6 or double
            if (tag >= STR0 && tag <= STR0 + 6) {
                return String::hashCode((char *) ptr, t.tag - STR0);                
            } else {
                return (w.w1 ^ w.w2) * FNV;
            }
        }
    }

    void setObj(Object *p) {
        ptr = p;
    }

    Object *getObj() {
        return ptr;
    }

    void setDouble(double v) {
        d = v;
        w.w2 ^= 0xffffffff;
    }

    double getDouble() {
        Value tmp;
        tmp.w.w1 = w.w1;
        tmp.w.w2 = ~w.w2;        
        return tmp.d;
    }

    void setInt(int v) {
        intVal = v;
        t.tag = INT;
    }

    void setString(int len, const char *s);
    int getString(char *s);
};

extern const Value NIL;
*/
