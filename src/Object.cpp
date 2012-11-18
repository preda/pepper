#include "Object.h"
#include "String.h"

unsigned Object::hashCode() {
    return type==STRING ? ((String *) this)->hashCode() : PTR_HASH(this);
}
