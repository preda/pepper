#include "Array.h"
#include "Map.h"
#include "Func.h"
#include "Proto.h"
#include "CFunc.h"
#include "String.h"

#include "GC.h"
#include <new>

Array *Array::alloc(int iniSize) {
    return new (GC::alloc(O_ARRAY, sizeof(Array), true)) Array(iniSize);
}

void Array::traverse() {
    GC::markVector(vect.buf, vect.size);
}

Map *Map::alloc(unsigned iniSize) {
    return new (GC::alloc(O_MAP, sizeof(Map), true)) Map(iniSize);
}

void Map::traverse() {
    GC::markVector(buf, size);
    GC::markVector(buf+(n>>1), size);
}

Func *Func::alloc(Proto *proto, Value *contextUps, Value *regs) {
    return new (GC::alloc(O_FUNC, sizeof(Func), true)) Func(proto, contextUps, regs);
}

void Func::traverse() { 
    GC::markVector(ups, proto->ups.size);
    GC::mark((Object *) proto);
}

Proto *Proto::alloc(Proto *up) {
    return new (GC::alloc(O_PROTO, sizeof(Proto), true)) Proto(up);
}

void Proto::traverse() {
    GC::mark((Object*) up);
    GC::markVector(consts.buf, consts.size);
}

CFunc *CFunc::alloc(tfunc f, int dataSize) {
    return new (GC::alloc(O_CFUNC, sizeof(CFunc) + dataSize, true)) CFunc(f);
}

String *String::alloc(unsigned size) {
    String *s = (String *) GC::alloc(O_STR, sizeof(String) + size + 1, false);
    s->size = size;
    return s;
}
