// Copyright (C) 2013 Mihai Preda

#include "StringBuilder.h"
#include "String.h"
#include "Array.h"
#include "Map.h"
#include "Object.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

StringBuilder::StringBuilder() :
    buf(0),
    size(0),
    allocSize(0)
    {}

StringBuilder::~StringBuilder() {
    if (buf) {
        free(buf);
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
        snprintf(tmp, sizeof(tmp), "%.17g", d);
    }
    append(tmp);
}

const char *typeStr(Value v) {
    const char *s = "?";
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
    } else if (IS_FUNC(v)) {
        s = "func";
    } else if (IS_CFUNC(v)) {
        s = "cfunc";
    } else if (IS_CF(v)) {
        s = "cf";
    } else if (IS_CP(v)) {
        s = "cp";
    } else if (IS_PROTO(v)) {
        s = "proto";
    } else if (IS_REG(v)) {
        s = "reg";
    }
    return s;
}

void StringBuilder::append(Value v, bool raw) {
    void *ptr = GET_PTR(v);
    if (IS_STRING(v)) {
        if (!raw) { append('\''); }
        append(GET_CSTR(v), len(v));
        if (!raw) { append('\''); }
    } else if (IS_NUM(v)) {
        append(GET_NUM(v));
        return;
    } else if (IS_ARRAY(v)) {
        Array *a = (Array *) ptr;
        int size = a->size();
        append('[');
        for (int i = 0; i < size; ++i) {
            append(i ? ", " : "");
            append(a->getI(i));
        }
        append(']');
    } else if (IS_MAP(v)) {
        Map *m = (Map *) ptr;
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
    } else if (IS_NIL(v)) {
        append("nil");
    } else {
        append('<');
        append(typeStr(v));
        append('>');
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%p", ptr);
        append(tmp);
    }
}
