// Copyright (C) 2012-2013 Mihai Preda

#include "SymbolTable.h"
#include <stdio.h>

SymbolTable::SymbolTable(GC *gc) : 
    level(0)
{
}

SymbolTable::~SymbolTable() {
}

int SymbolTable::pushContext() {
    // printf("SymbolTable push level %d\n", level+1);
    return ++level;    
}

void SymbolTable::undo(UndoEntry *p) {
    map.rawSet(p->name, p->prev);
}

int SymbolTable::popContext() {
    Vector<UndoEntry> *currUndo = undoLog + level;
    int n = currUndo->size();
    UndoEntry *buf = currUndo->buf();
    for (UndoEntry *p = buf + n - 1; p >= buf; --p) {
        undo(p);
    }
    // printf("SymbolTable pop level %d\n", level);
    return --level;
}

Value SymbolTable::get(Value name) {
    return map.rawGet(name);
}

void SymbolTable::set(Value name, int slot, int level) {
    if (level == -1) { level = this->level; }
    Value prev = map.rawGet(name);
    if (IS_NIL(prev) || ((prev & 0xff) != (unsigned) level)) {
        UndoEntry *u = (undoLog + level)->push();
        u->name = name;
        u->prev = prev;
    }
    map.rawSet(name, VAL_REG(((slot<<8) | level)));
}
