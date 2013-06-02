#pragma once

#include "Value.h"
#include "Proto.h"

class GC;

class Func {
    Func(Proto *proto, Value *contextUps, Value *regs, byte recSlot);
    byte type;

 public:
    Proto *proto;
    Value *ups;

    static Func *alloc(GC *gc, Proto *proto, Value *contextUps, Value *regs, byte recSlot);
    ~Func();

    unsigned nUp() { return proto->nUp(); }
    void setUp(int slot, Value v);
    
    void traverse(GC *gc);
};
