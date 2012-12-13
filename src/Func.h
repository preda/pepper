#pragma once

#include "Value.h"
#include "Proto.h"

class Func {
    Func(Proto *proto, Value *contextUps, Value *regs, byte recSlot);
    byte type;

 public:
    Proto *proto;
    Value *ups;

    unsigned nUp() { return proto->nUp(); }
    unsigned nOwnUp() { return proto->nUp() - N_CONST_UPS; }

    static Func *alloc(Proto *proto, Value *contextUps, Value *regs, byte recSlot);
    ~Func();

    void traverse();
};
