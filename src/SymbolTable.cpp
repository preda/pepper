#include "SymbolTable.h"

#include <stdio.h>

int SymbolTable::popContext() {
    SymbolVect *currUndo = undoLog + level;
    int n = currUndo->size();
    Symbol *buf = currUndo->buf();
    for (Symbol *s = buf + n - 1; s >= buf; --s) {
        undo(s);
    }
    return --level;
}

SymbolTable::SymbolTable() : level(0) {
}

SymbolTable::~SymbolTable() {
}

void SymbolTable::print() {
}
