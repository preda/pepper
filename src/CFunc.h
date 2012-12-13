#pragma once

#include "Value.h"

typedef void (*tfunc)(int op, void *data, Value *stack, int nCallArg);

class CFunc {
    byte type;
    tfunc cfunc;

    CFunc(tfunc f);

enum {
    CFUNC_CALL   = 0,
    CFUNC_INIT   = 1,
    CFUNC_DELETE = 2,
    CFUNC_GC_TRAVERSE = 3,
};

 public:
    byte data[0];

    ~CFunc();

    static CFunc *alloc(tfunc f, unsigned dataSize = 0);

    void traverse() { cfunc(CFUNC_GC_TRAVERSE, data, 0, 0); }

    void call(Value *stack, int nCallArg) {
        cfunc(CFUNC_CALL, data, stack, nCallArg);
    }
};
