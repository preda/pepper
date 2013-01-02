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

const char *Object::getTypeName(int type) {
    const char *names[] = {"UNDEF", "ARRAY", "MAP", "FUNC", "CFUNC", "PROTO", "STR"};
    return names[type];
}
