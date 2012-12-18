#pragma once

#include "Value.h"
#include <ffi.h>

class GC;

struct FFIData {
    bool hasEllipsis;
    byte nArg;
    void (*func)();
    ffi_cif cif;
    byte retType;
    byte argType[8];
    ffi_type *ffiArgs[8];
    GC *gc;
};

Value ffiCall(GC *gc, int op, FFIData *data, Value *stack, int nCallArg);
Value ffiConstruct(GC *gc, int op, void *data, Value *stack, int nCallArg);
