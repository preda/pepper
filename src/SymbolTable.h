// Copyright (C) 2012 Mihai Preda

#pragma once

#include "Vector.h"
#include "common.h"

class Map;
class GC;

class SymbolTable {
    struct UndoEntry {
        Value name;
        Value prev;
    };

    typedef Vector<UndoEntry> UndoVect;

    int level;
    UndoVect undoLog[16];
    Map *map;

    void undo(UndoEntry *p);
    
 public:
    SymbolTable(GC *gc);
    ~SymbolTable();

    int pushContext();
    int popContext();
    
    Value get(Value name);
    void set(Value name, int slot, int level = -1);
};
