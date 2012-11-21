#pragma once

#include "Value.h"

struct FFIData {
    void *f;
    bool hasEllipsis;
    byte retType;
    short  nArg;
    byte argType[8];
};

enum CTypes {
    CVOID = 0,
    CCHAR,
    CINT,
    CLONG,
    CLONGLONG,
    CDOUBLE,
    CELLIPSE,
    PTRDIFF,
    CPTR   = 16,
    CPTR2  = 32,
    CCONST = 64
};

void ffiCall(int op, FFIData *data, Value *stack, int nCallArg);
void ffiConstruct(int op, void *data, Value *stack, int nCallArg);
