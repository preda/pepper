// Copyright (C) 2012 Mihai Preda

#pragma once

#include "value.h"

class GC;
class VM;
class Func;
class SymbolTable;

class Pepper {
    GC *gc;
    VM *vm;
    SymbolTable *syms;

 public:
    Pepper(void *context);
    ~Pepper();

    Func *parseFunc(const char *text);
    Func *parseStatList(const char *text);
    GC *getGC() { return gc; }
    Value run(Func *f, int nArg = 0, Value *args = 0);
};
