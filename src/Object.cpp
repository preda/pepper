#include "Object.h"
#include "String.h"

unsigned Object::hashCode() {
    return type==O_STR ? ((String *) this)->hashCode() : PTR_HASH(this);
}
