// this is the only file that uses "placement new", including <new>

#include "Array.h"
#include "Map.h"
#include "Func.h"
#include "Proto.h"
#include "CFunc.h"
#include "String.h"
#include "GC.h"
#include <new>

Array *Array::alloc(GC *gc, int iniSize) {
    return new (gc->alloc(O_ARRAY, sizeof(Array), true)) Array(iniSize);
}

Map *Map::alloc(GC *gc, unsigned iniSize) {
    return new (gc->alloc(O_MAP, sizeof(Map), true)) Map(iniSize);
}

Func *Func::alloc(GC *gc, Proto *proto, Value *contextUps, Value *regs, byte recSlot) {
    return new (gc->alloc(O_FUNC, sizeof(Func), true)) Func(proto, contextUps, regs, recSlot);
}

Proto *Proto::alloc(GC *gc, Proto *up) {
    return new (gc->alloc(O_PROTO, sizeof(Proto), true)) Proto(up);
}

CFunc *CFunc::alloc(GC *gc, tfunc f, unsigned dataSize) {
    return new (gc->alloc(O_CFUNC, sizeof(CFunc) + dataSize, dataSize != 0)) CFunc(f);
}
