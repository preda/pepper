#pragma once

#include "Value.h"

typedef void (*tfunc)(int op, void *data, Value *stack, int nCallArg);

class CFunc {
    CFunc(tfunc f);

 public:
    byte type;
    tfunc func;
    byte data[0];
    
    ~CFunc();
    static CFunc *alloc(tfunc f, int dataSize);
    void traverse() { func(1, data, 0, 0); }

    void call(Value *stack, int nCallArg) {
        func(0, data, stack, nCallArg);
    }
};
