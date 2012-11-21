#include "Value.h"
#include "String.h"
#include "Object.h"
#include <assert.h>
// #include <string.h>
// #include <stdio.h>

unsigned hashCode(Value a) {
    if (IS_STRING(a)) {
        return String::hashCode(GET_CSTR(a), len(a));
    } else {
        switch (TAG(a)) {
        case T_OBJ:  return a ? ((Object *)a)->hashCode() : 0;
        case T_INT:  return (unsigned) a * FNV;
        default: return (((unsigned) a) ^ ((unsigned) (a >> 32))) * FNV;
        }        
    }
    assert(false);
}

unsigned len(Value a) {
    if (IS_SHORT_STR(a)) { return SHORT_STR_LEN(a); }
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
