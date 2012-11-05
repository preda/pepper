#pragma once

#include "Vector.h"
#include "Value.h"

struct Proto {
    byte nArgs;
    bool hasEllipsis;
    Vector<short> ups;
    Vector<Value> consts;
    Vector<unsigned> code;
    Proto *parent;
};

struct Func {
    byte type;
    Proto *proto;
    Value *ups;

    static Func *alloc(Proto *proto, Func *parent, Value *regs);

    Func(Proto *proto, Func *context, Value *regs);    
    void destroy();
    void traverse();
};
