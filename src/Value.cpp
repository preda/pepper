#include "Value.h"
#include "String.h"
#include "Object.h"

#include <string.h>
#include <stdio.h>

Value VAL_STRING(const char *s, int len) {
    if (len <= 6) {
        Value v = VALUE(STRING+len, 0);
        memcpy(&v, s, len);
        return v;
    } else {
        return VAL_OBJ(String::alloc(s, len));
    }
}

unsigned hashCode(Value a) {
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

unsigned len(Value a) {
    const int t = TAG(a);
    return t>STRING && t<=STRING+6 ? t-STRING : t==OBJECT ? ((Object *) a)->size : 0;
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
