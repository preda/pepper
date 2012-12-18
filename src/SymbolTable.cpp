#include "SymbolTable.h"

#include <stdio.h>

int SymbolTable::pushContext() {
    printf("SymbolTable push level %d\n", level+1);
    return ++level;    
}

int SymbolTable::popContext() {
    SymbolVect *currUndo = undoLog + level;
    int n = currUndo->size();
    Symbol *buf = currUndo->buf();
    for (Symbol *s = buf + n - 1; s >= buf; --s) {
        undo(s);
    }
    printf("SymbolTable pop level %d\n", level);
    return --level;
}

SymbolTable::SymbolTable() : level(0) {
}

SymbolTable::~SymbolTable() {
}

void SymbolTable::print() {
}
