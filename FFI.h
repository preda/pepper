#pragma once

#include "CFunc.h"

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
    CPTR   = 16,
    CPTR2  = 32,
    CCONST = 64
};

int ffiCall(int op, byte *data, Value *stack, int nCallArg);
int ffiConstruct(int op, byte *data, Value *stack, int nCallArg);
