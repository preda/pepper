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
    Map map;
    int level;
    Vector<UndoEntry> undoLog[16];
    void undo(UndoEntry *p);
    
 public:
    SymbolTable(GC *gc);
    ~SymbolTable();

    int pushContext();
    int popContext();
    
    Value get(Value name);
    void set(Value name, int slot, int level = -1);
};
