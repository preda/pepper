// Copyright (C) 2012-2013 Mihai Preda

#pragma once

#include "common.h"
#include "Map.h"
#include "Vector.h"

class GC;

struct UndoEntry {
    Value name;
    Value prev;
};

// Object
class SymbolTable {
    int starts[16];
    Vector<Value> names;
    Vector<int> slots;

    int getLevel(int pos);
    int findPos(Value name);
    
 public:
    int curLevel;
    
    SymbolTable(GC *gc);
    ~SymbolTable();

    int pushContext();
    int popContext();
    
    Value get(Value name);
    void set(Value name, int slot);
    void set(Value name, int slot, int level);
};
