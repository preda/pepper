#pragma once

#include "Value.h"

class Proto;

class Func {
 public:
    byte type;
    Proto *proto;
    Value *ups;

    static Func *alloc(Proto *proto, Func *parent, Value *regs);

    Func(Proto *proto, Func *context, Value *regs);    
    void destroy();
    void traverse();
};
