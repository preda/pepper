#include "String.h"
#include "Object.h"
#include "GC.h"

#include <string.h>
#include <assert.h>

String::~String() {
}

String *String::alloc(int size) {
    String *s = (String *) GC::alloc(O_STR, sizeof(String) - 4 + size, false);
    s->size = size;
    return s;
}

String *String::alloc(const char *p, int size) {
    String *s = alloc(size);
    memcpy(s->s, p, size);
    return s;
}

Value String::get(Value s, Value p) {
    assert(IS_STRING(s));
    ERR(!IS_INT(p), E_INDEX_NOT_INT);
    s64 pos = getInteger(p);
    unsigned size = len(s);
    if (pos < 0) { pos += size; }
    if (pos >= size || pos < 0) { return NIL; }
    return VAL_STRING(GET_CSTR(s) + (unsigned)pos, 1);
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

static Value stringConcat(Value a, char *pb, int sb) {
    int ta = TAG(a);
    char *pa;
    int sa;
    Value ret;

    if (IS_SHORT_STR(a)) {
        sa = ta - T_STR;
        pa = (char *) &a;
    } else {
        String *s = (String *) a;
        sa = s->size;
        pa = s->s;
    }
    char *pr;
    if (sa + sb <= 6) {
        ret = VALUE(T_STR + (sa+sb), 0);
        pr = (char *) &ret;
    } else {
        String *s = String::alloc(sa + sb);
        pr = s->s;
        ret = VAL_OBJ(s);
    }
    memcpy(pr, pa, sa);
    memcpy(pr+sa, pb, sb);
    return ret;    
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
    int tb = TAG(b);
    int sb;
    char *pb;
    if (IS_SHORT_STR(b)) {
        sb = tb - T_STR;
        pb = (char *) &b;
    } else if (IS_NUMBER(b)) {
        numberToString(b, buf, sizeof(buf));
        sb = strlen(buf);
        pb = buf;
    } else if (IS_STRING(b)) {    
        String *s = (String *) b;
        sb = s->size;
        pb = s->s;
    } else {
        ERR(true, E_STR_ADD_TYPE);
        // error();
        return NIL;
    }
    return stringConcat(a, pb, sb);
}
