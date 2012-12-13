#include "Value.h"
#include "String.h"
#include "Object.h"
#include <assert.h>

unsigned hashCode(Value a) {
    if (IS_STRING(a)) {
        return String::hashCode(GET_CSTR(a), len(a));
    } else {
        switch (TAG(a)) {
        case T_OBJ: return a ? GET_OBJ(a)->hashCode() : 0;
        case T_NIL: return 0;
        default:    return (((unsigned) a) ^ ((unsigned) (a >> 32))) * FNV;
        }        
    }
    assert(false);
}

unsigned len(Value a) {
    if (IS_SHORT_STR(a)) { return SHORT_STR_LEN(a); }
    ERR(!(IS_ARRAY(a) || IS_STRING(a) || IS_MAP(a)), E_LEN_NOT_COLLECTION);
    return GET_OBJ(a)->size();
}
