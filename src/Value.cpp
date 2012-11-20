#include "Value.h"
#include "String.h"
#include "Object.h"

#include <string.h>
#include <stdio.h>

Value VAL_STRING(const char *s, int len) {
    if (len <= 6) {
        Value v = VALUE(T_STR+len, 0);
        memcpy(&v, s, len);
        return v;
    } else {
        return VAL_OBJ(String::alloc(s, len));
    }
}

unsigned hashCode(Value a) {
    int tag = TAG(a);
    switch (tag) {
    case T_OBJ:  return a ? ((Object *)a)->hashCode() : 0;
    case T_INT:  return (unsigned) a * FNV;
    default: // STR0-6 or double        
        if (IS_SHORT_STR(a)) {
            Value b = a;
            return String::hashCode((char *)&b, tag-T_STR);
        } else {
            return (((unsigned) a) ^ ((unsigned) (a >> 32))) * FNV;
        }
    }
}

unsigned len(Value a) {
    if (IS_SHORT_STR(a)) { return TAG(a) - T_STR; }
    ERR(!(IS_ARRAY(a) || IS_STRING(a) || IS_MAP(a)), E_LEN_NOT_COLLECTION);
    return ((Object *) a)->size;
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
