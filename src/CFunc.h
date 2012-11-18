#pragma once

#include "Value.h"

typedef int (*tfunc)(int op, byte *data, Value *stack, int nCallArg);

class CFunc {

 public:
    byte type;
    tfunc func;
    byte data[0];
    
    ~CFunc();
    static CFunc *alloc(void *f, int dataSize);
    void traverse() { func(1, data, 0, 0); }

    int call(Value *stack, int nCallArg) {
        return func(0, data, stack, nCallArg);
    }
};
