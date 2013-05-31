// Copyright (C) 2012-2013 Mihai Preda

#pragma once

#include "common.h"
#include "Map.h"
#include "Vector.h"

class GC;

// Object
class SymbolTable {
    Vector<int> protos;
    Vector<int> starts;
    Vector<int> slots;
    Vector<Value> names;

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
