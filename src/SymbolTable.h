// Copyright (C) 2012-2013 Mihai Preda

#pragma once

#include "common.h"
#include "Map.h"
#include "Vector.h"
#include "Array.h"

class GC;

// Object
class SymbolTable {
    // Vector<Value> names;
    Array names;
    Vector<int> protos;
    Vector<int> starts;
    Vector<int> slots;

    int getLevel(int pos);
    int findPos(Value name);
    
 public:
    SymbolTable();
    ~SymbolTable();

    void enterBlock(bool isProto);
    void exitBlock(bool isProto);
    
    Value get(Value name);
    void set(Value name, int slot);
    void setUpval(Value name, int slot, int level);
    int curLevel() { return protos.size() - 1; }
};
