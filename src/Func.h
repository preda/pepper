#pragma once

#include "Value.h"

class Proto;

class Func {
 private:
    Func(Proto *proto, Value *contextUps, Value *regs);

 public:
    byte type;
    Proto *proto;
    Value *ups;

    static Func *alloc(Proto *proto, Value *contextUps, Value *regs);
    ~Func();

    void traverse();
};
