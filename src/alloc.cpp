// Copyright (C) 2012-2013 Mihai Preda

// this is the only file that uses "placement new"
// and must include the <new> header

#include "Array.h"
#include "Map.h"
#include "Func.h"
#include "Proto.h"
#include "CFunc.h"
#include "String.h"
#include "GC.h"
#include "SymbolTable.h"

#include <assert.h>
#include <new>

/*
SymbolTable *SymbolTable::alloc(GC *gc) {
    return new (gc->alloc(sizeof(SymbolTable), true)) SymbolTable;
}
*/

Array *Array::alloc(GC *gc, int iniSize) {
    return new (gc->alloc(sizeof(Array), true)) Array(iniSize);
}

Map *Map::alloc(GC *gc, unsigned sizeHint) {
    (void) sizeHint;
    return new (gc->alloc(sizeof(Map), true)) Map;
}

Func *Func::alloc(GC *gc, Proto *proto, Value *contextUps, Value *regs, byte recSlot) {
    return new (gc->alloc(sizeof(Func), true)) Func(proto, contextUps, regs, recSlot);
}

Proto *Proto::alloc(GC *gc, Proto *up) {
    return new (gc->alloc(sizeof(Proto), true)) Proto(up);
}

CFunc *CFunc::alloc(GC *gc, tfunc f, unsigned dataSize) {
    assert(dataSize);
    return new (gc->alloc(sizeof(CFunc) + dataSize, true)) CFunc(f);
}

SymbolTable *SymbolTable::alloc(GC *gc) {
    return new (gc->alloc(sizeof(SymbolTable), true)) SymbolTable();
}
