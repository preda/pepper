// Copyright (C) 2012 - 2013 Mihai Preda

#include "builtin.h"
#include "Value.h"
#include "Array.h"
#include "Map.h"
#include "Object.h"
#include "String.h"
#include "VM.h"
#include "CFunc.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

class StringBuilder {
public:
    char *buf;
    int size, allocSize;

    StringBuilder();
    ~StringBuilder();

    void reserve(int space);
    void clear() { size = 0; }
    char *cstr();

    void append(const char *s);
    void append(const char *s, int len);
    void append(char c);
    void append(double d);
    void append(Value v);
};

StringBuilder::StringBuilder() :
    buf(0),
    size(0),
    allocSize(0)
    {}

StringBuilder::~StringBuilder() {
    if (buf) {
        delete buf;
    }
    buf = 0;
    size = allocSize = 0;
}

char *StringBuilder::cstr() {
    reserve(1);
    buf[size] = 0;
    return buf;
}

void StringBuilder::reserve(int space) {
    while (size + space >= allocSize) {
        allocSize += allocSize ? allocSize : 32;
        buf = (char *) realloc(buf, allocSize);
    }
}

void StringBuilder::append(const char *s) {
    append(s, strlen(s));
}

void StringBuilder::append(const char *s, int len) {
    reserve(len);
    memcpy(buf + size, s, len);
    size += len;
}

void StringBuilder::append(char c) {
    reserve(2);
    buf[size] = c;
    buf[size + 1] = 0;
    size += 1;    
}

void StringBuilder::append(double d) {
    char tmp[32];
    if (d == (long long) d) {
        snprintf(tmp, sizeof(tmp), "%lld", (long long) d);
    } else {
        snprintf(tmp, sizeof(tmp), "%f", d);
    }
    append(tmp);
}

void StringBuilder::append(Value v) {
    if (IS_STRING(v)) {
        append(GET_CSTR(v), len(v));
    } else if (IS_NUM(v)) {
        append(GET_NUM(v));
    } else if (IS_OBJ(v)) {
        Object *p = GET_OBJ(v);
        if (IS_ARRAY(v)) {
            Array *a = (Array *) p;
            int size = a->size();
            append('[');
            for (int i = 0; i < size; ++i) {
                append(i ? ", " : "");
                append(a->getI(i));
            }
            append(']');
        } else if (IS_MAP(v)) {
            Map *m = (Map *) p;
            int size = m->size();
            append('{');
            Value *keys = m->keyBuf();
            Value *vals = m->valBuf();
            for (int i = 0; i < size; ++i) {
                append(i ? ", " : "");
                append(keys[i]);
                append(':');
                append(vals[i]);
            }
            append('}');
        } else {
            char tmp[32];
            snprintf(tmp, sizeof(tmp), "%p", p);
            append(tmp);
        }
    } else if (IS_NIL(v)) {
        append("NIL");
    } else if (IS_CF(v)) {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "CF %p", GET_CF(v));
        append(tmp);
    } else if (IS_CP(v)) {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "CP %p", GET_CF(v));
        append(tmp);
    }
}

Value builtinPrint(VM *vm, int op, void *data, Value *stack, int nCallArgs) {
    assert(op == CFunc::CFUNC_CALL && !data);
    assert(nCallArgs > 0);
    StringBuilder buf;    
    for (int i = 1; i < nCallArgs; ++i) {
        buf.clear();
        buf.append(stack[i]);
        printf("%s ", buf.cstr());
    }
    printf("\n");
    return VNIL;    
}

Value builtinType(VM *vm, int op, void *data, Value *stack, int nCallArgs) {
    assert(op == CFunc::CFUNC_CALL && !data);
    assert(nCallArgs > 0);
    if (nCallArgs < 2) { return VNIL; }
    Value v = stack[1];
    const char *s = 0;
    if (IS_NIL(v)) {
        s = "nil";
    } else if (IS_NUM(v)) {
        s = "number";
    } else if (IS_STRING(v)) {
        s = "string";
    } else if (IS_ARRAY(v)) {
        s = "array";
    } else if (IS_MAP(v)) {
        s = "map";
    } else if (IS_FUNC(v) || IS_CFUNC(v)) {
        s = "func";
    }
    // else if (IS_PROTO(v)) { s = "proto";
    assert(s); // unexpected type
    return String::value(vm->getGC(), s);
}

Value builtinGC(VM *vm, int op, void *data, Value *stack, int nCallArg) {
    assert(op == CFunc::CFUNC_CALL && !data);
    assert(nCallArg > 0);
    // vm->gcCollect(stack + nCallArg);
    return VNIL;
}

Value builtinImport(VM *vm, int op, void *data, Value *stack, int nCallArg) {
    assert(op == CFunc::CFUNC_CALL && !data);
    assert(nCallArg > 0);
    if (nCallArg >= 2) {
        Value v = stack[1];
        if (IS_STRING(v)) {
            // TODO
        }
    }
    return VNIL;
}

#ifdef __ANDROID__
#include "JavaLink.h"
Value javaClass(VM *vm, int op, void *data, Value *stack, int nCallArg) {
    if (op != CFunc::CFUNC_CALL) { return VNIL; }
    if (nCallArg < 2) { return VNIL; }
    Value v = stack[1];
    if (!IS_STRING(v)) { return VNIL; }
    const char *name = GET_CSTR(v);
    if (!name) { return VNIL; }
    JavaLink *java = (JavaLink *)(vm->getContext());
    if (!java) { return VNIL; }
    JNIEnv *env = java->env;
    if (!env) { return VNIL; }
    // return VNIL;
    jclass cls = env->FindClass("java/lang/String");
    return Map::makeMap(vm->getGC(), "_class", VAL_CP(cls), 0); 
    // return String::value(vm->getGC(), name);
}
#else
Value javaClass(VM *vm, int op, void *data, Value *stack, int nCallArg) {
    return VNIL;
}
#endif
