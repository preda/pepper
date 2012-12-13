#include "Object.h"
#include "String.h"
#include <assert.h>

unsigned Object::hashCode() {
    return type()==O_STR ? ((String *) this)->hashCode() : PTR_HASH(this);
}

void Object::setType(unsigned t) {
    assert(!type());
    assert(t <= 7);
    _size |= t;
}
