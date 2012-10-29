#include "Value.h"
#include <string.h>
#include <stdio.h>

static Value stringConcat(Value a, char *pb, int sb) {
    int ta = TAG(a);
    char *pa;
    int sa;
    Value ret;

    if (ta>=STRING && ta<=STRING+6) {
        sa = ta - STRING;
        pa = (char *) &a;
    } else {
        String *s = (String *) a;
        sa = s->size;
        pa = s->s;
    }
    char *pr;
    if (sa + sb <= 6) {
        ret = VALUE(STRING+(sa+sb), 0);
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
    if (TAG(a)==INTEGER) {
        snprintf(out, outSize, "%d", (int)(unsigned)a);
    } else {
        snprintf(out, outSize, "%.17g", getDouble(a));
    }    
}

Value stringConcat(Value a, Value b) {
    char buf[32];
    int tb = TAG(b);
    int sb;
    char *pb;
    if (tb>=STRING && tb<=STRING+6) {
        sb = tb - STRING;
        pb = (char *) &b;
    } else if (IS_NUMBER(b)) {
        numberToString(C, buf, sizeof(buf));
        sb = strlen(buf);
        pb = buf;
    } else if (IS_STRING(b)) {    
        String *s = (String *) b;
        sb = s->size;
        pb = s->s;
    } else {
        return ERR;
    }
    return stringConcat(a, pb, sb);
}

/*
void Value::setString(int len, const char *s) {
    t.tag = STR0 + len;
    memcpy(ptr, s, len);
}
    
int Value::getString(char *s) {
    int len = t.tag - STR0;
    memcpy(s, ptr, len);
    return len;
}
*/
