#pragma once

#include "Value.h"
#include <ffi.h>

struct FFIData {
    bool hasEllipsis;
    byte nArg;
    void (*func)();
    ffi_cif cif;
    byte retType;
    byte argType[8];
    ffi_type *ffiArgs[8];
};

Value ffiCall(int op, FFIData *data, Value *stack, int nCallArg);
Value ffiConstruct(int op, void *data, Value *stack, int nCallArg);
