#pragma once

#include "Value.h"
#include <ffi.h>

class GC;
class VM;

struct FFIData {
    bool hasEllipsis;
    byte nArg;
    void (*func)();
    ffi_cif cif;
    byte retType;
    byte argType[8];
    ffi_type *ffiArgs[8];
};

Value ffiCall(VM *vm, int op, FFIData *data, Value *stack, int nCallArg);
Value ffiConstruct(VM *vm, int op, void *data, Value *stack, int nCallArg);
