#include "Object.h"
#include "Array.h"
#include "Map.h"
#include "String.h"
#include "CFunc.h"

void Object::traverse() {
    switch (type) {
    case CFUNC: ((CFunc *) this)->traverse(); break;
    case ARRAY: ((Array *) this)->traverse(); break;
    case MAP:   ((Map *)   this)->traverse(); break;
    case FUNC: break;
    default: break;
    }
}

void Object::destroy() {
    switch (type) {
    case CFUNC: ((CFunc *) this)->destroy(); break;
    case ARRAY: ((Array *) this)->destroy(); break;
    case MAP:   ((Map *)   this)->destroy(); break;
    }
}

unsigned Object::hashCode() {
    return type==STRING ? ((String *) this)->hashCode() : PTR_HASH(this);
}
