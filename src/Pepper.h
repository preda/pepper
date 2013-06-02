// Copyright (C) 2012 Mihai Preda

#pragma once

#include "value.h"

class GC;
class VM;
class Func;
class SymbolTable;
class Array;

class Pepper {
    GC *_gc;
    VM *vm;
    SymbolTable *_syms;
    Array *_regs;

 public:
    Pepper(void *context);
    ~Pepper();

    Func *parseFunc(const char *text);
    Func *parseStatList(const char *text);
    GC *gc() { return _gc; }
    SymbolTable *syms() { return _syms; }
    Value *regs();
    
    Value run(Func *f, int nArg = 0, Value *args = 0);
};
