#include "String.h"
#include "Object.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

Value String::makeVal(unsigned len) {
    return len <= 6 ? VALUE(T_STR0 - len, 0) : VAL_OBJ(String::alloc(len));
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
    ERR(!IS_INT(p), E_INDEX_NOT_INT);
    s64 pos = getInteger(p);
    unsigned size = len(s);
    if (pos < 0) { pos += size; }
    if (pos >= size || pos < 0) { return NIL; }
    return VALUE(T_STR1, (u64)(GET_CSTR(s) + (unsigned)pos));
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
    return hashCode(s, size);
}

bool String::equals(String *other) {
    return size == other->size && *(int*)s==*(int*)other->s && !memcmp(s, other->s, size);
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
    if (IS_INT(a)) {
        snprintf(out, outSize, "%d", (int)(unsigned)a);
    } else {
        snprintf(out, outSize, "%.17g", getDouble(a));
    }    
}

Value String::concat(Value a, Value b) {
    char buf[32];
    int sb;
    char *pb;    
    if (IS_NUMBER(b)) {
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
