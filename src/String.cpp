#include "String.h"
#include "Object.h"
#include "Map.h"
#include "CFunc.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

Map *String::methods;

void String::staticInit() {
    methods = Map::alloc();
    methods->set(makeVal("find"), VAL_OBJ(CFunc::alloc(method_find)));
}

Value String::makeVal(unsigned len) {
    return len <= 5 ? VAL_TAG(T_STR0 + len) : VAL_OBJ(String::alloc(len));
}

Value String::makeVal(const char *s) {
    return makeVal(s, strlen(s));
}

Value String::makeVal(const char *s, unsigned len) {
    Value v = makeVal(len);
    char *p = GET_CSTR(v);
    memcpy(p, s, len);
    p[len] = 0;    
    return v;
}

String::~String() {
}

String *String::alloc(const char *p, unsigned size) {
    String *s = alloc(size);
    memcpy(s->s, p, size);
    s->s[size] = 0;
    return s;
}

Value String::get(Value s, Value p) {
    assert(IS_STRING(s));
    ERR(!IS_NUM(p), E_INDEX_NOT_NUMBER);
    s64 pos = (s64) GET_NUM(p);
    unsigned size = len(s);
    if (pos < 0) { pos += size; }
    if (pos >= size || pos < 0) { return NIL; }
    return VALUE_(T_STR1, (unsigned char) *(GET_CSTR(s) + (unsigned)pos));
}

Value String::getSlice(Value s, Value p1, Value p2) {
    assert(IS_STRING(s));
    ERR(!IS_NUM(p1) || !IS_NUM(p2), E_INDEX_NOT_NUMBER);
    s64 pos1 = (s64) GET_NUM(p1);
    s64 pos2 = (s64) GET_NUM(p2);
    unsigned size = len(s);
    if (pos1 < 0) { pos1 += size; }
    if (pos2 < 0) { pos2 += size; }
    if (pos1 < 0) { pos1 = 0; }
    if (pos2 > size) { pos2 = size; }
    // if (pos1 < 0 || pos2 >= size) { return NIL; }
    return makeVal(GET_CSTR(s) + pos1, (pos1 < pos2) ? (unsigned)(pos2-pos1) : 0);
}

unsigned String::hashCode(char *buf, int size) {
    unsigned char *p = (unsigned char *) buf;
    unsigned h = 0;
    unsigned char *end = p + size;
    while (p < end) {
        h = (h ^ *p++) * FNV;
    }
    return h;
}

unsigned String::hashCode() {
    return hashCode(s, size());
}

bool String::equals(String *other) {
    return _size == other->_size && !memcmp(s, other->s, size());
    // && *(int*)s==*(int*)other->s 
}

static Value stringConcat(Value a, char *pb, unsigned sb) {
    assert(IS_STRING(a));
    unsigned sa = len(a);
    Value v = String::makeVal(sa + sb);
    char *pv = GET_CSTR(v);
    char *pa = GET_CSTR(a);
    memcpy(pv, pa, sa);
    memcpy(pv + sa, pb, sb);
    pv[sa + sb] = 0;
    return v;
}

static void numberToString(Value a, char *out, int outSize) {
    snprintf(out, outSize, "%.17g", GET_NUM(a));
}

Value String::concat(Value a, Value b) {
    char buf[32];
    int sb;
    char *pb;    
    if (IS_NUM(b)) {
        numberToString(b, buf, sizeof(buf));
        pb = buf;
        sb = strlen(buf);
    } else {
        ERR(!IS_STRING(b), E_STR_ADD_TYPE);
        pb = GET_CSTR(b);
        sb = len(b);
    }
    return stringConcat(a, pb, sb);
}

Value String::method_find(int op, void *data, Value *stack, int nCallArg) {
    assert(nCallArg > 0);
    Value *p = stack, *end = p + nCallArg;
    Value v1 = *p++;
    if (v1 == NIL) {
        if (p >= end) { return NIL; }
        v1 = *p++;
    }
    if (p >= end) { return NIL; }
    Value v2 = *p++;
    if (!IS_STRING(v1) || !IS_STRING(v2)) {
        return NIL;
    }
    
    char *str1 = GET_CSTR(v1);
    char *str2 = GET_CSTR(v2);
    // unsigned len1 = len(v1);
    // unsigned len2 = len(v2);
    char *pos = strstr(str1, str2);
    return VAL_NUM(pos ? pos - str1 : -1);
}
