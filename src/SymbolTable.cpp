// Copyright (C) 2012 Mihai Preda

#include "SymbolTable.h"
#include "Map.h"

#include <stdio.h>

SymbolTable::SymbolTable(GC *gc) : 
    level(0),
    map(Map::alloc(gc))
{
}

SymbolTable::~SymbolTable() {
}

int SymbolTable::pushContext() {
    // printf("SymbolTable push level %d\n", level+1);
    return ++level;    
}

void SymbolTable::undo(UndoEntry *p) {
    map->set(p->name, p->prev);
}

int SymbolTable::popContext() {
    UndoVect *currUndo = undoLog + level;
    int n = currUndo->size();
    UndoEntry *buf = currUndo->buf();
    for (UndoEntry *p = buf + n - 1; p >= buf; --p) {
        undo(p);
    }
    // printf("SymbolTable pop level %d\n", level);
    return --level;
}

Value SymbolTable::get(Value name) {
    return map->get(name);
}

void SymbolTable::set(Value name, int slot, int level) {
    if (level == -1) { level = this->level; }
    Value prev = map->get(name);
    if (IS_NIL(prev) || ((prev & 0xff) != (unsigned) level)) {
        UndoEntry *u = (undoLog + level)->push();
        u->name = name;
        u->prev = prev;
    }
    map->set(name, VAL_REG(((slot<<8) | level)));
}
