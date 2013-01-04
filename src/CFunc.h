#pragma once

#include "Value.h"
class VM;
class GC;

typedef Value (*tfunc)(VM *vm, int op, void *data, Value *stack, int nCallArg);

class CFunc {
    byte type;
    tfunc cfunc;

    CFunc(tfunc f);

 public: 
    enum {
        CFUNC_CALL   = 0,
        CFUNC_INIT   = 1,
        CFUNC_DELETE = 2,
        CFUNC_GC_TRAVERSE = 3,
    };

    byte data[0];

    ~CFunc();

    static CFunc *alloc(GC *gc, tfunc f, unsigned dataSize = 0);
    static Value value(GC *gc, tfunc f);

    void traverse(GC *gc) { cfunc(0, CFUNC_GC_TRAVERSE, data, (Value *) gc, 0); }

    void call(VM *vm, Value *stack, int nCallArg) {
        *stack = cfunc(vm, CFUNC_CALL, data, stack, nCallArg);
    }
};
