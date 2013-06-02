// Copyright (C) 2012-2013 Mihai Preda

#pragma once

#include "Vector.h"
#include "value.h"

class GC;
class Array;

// Object
class SymbolTable {
    Vector<Value> names;
    Vector<int> protos;
    Vector<int> starts;
    Vector<int> slots;

    SymbolTable();
    int getLevel(int pos);
    int findPos(Value name);

 public:
    static SymbolTable *alloc(GC *gc);
    ~SymbolTable();

    void traverse(GC *gc); // GC
    void enterBlock(bool isProto);
    void exitBlock(bool isProto);
    
    Value get(Value name);
    void set(Value name, int slot);
    void setUpval(Value name, int slot, int level);
    int curLevel() { return protos.size() - 1; }

    void add(GC *gc, Array *regs, const char *name, Value v);
};
