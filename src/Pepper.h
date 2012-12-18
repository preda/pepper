// Copyright (C) 2012 Mihai Preda

#pragma once

#include "Value.h"

class GC;
class SymbolTable;
class VM;
class Func;

class Pepper {
    GC *gc;
    SymbolTable *syms;
    Value *ups, *upsTop;
    VM *vm;
        
 public:
    Value builtinString;
    Value EMPTY_ARRAY;
    Value EMPTY_MAP;

    Pepper();
    ~Pepper();

    Func *parseFunc(const char *text);
    Func *parseStatList(const char *text);
    GC *getGC() { return gc; }
    Value run(Func *f, int nArg = 0, Value *args = 0);
};
