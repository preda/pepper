// Copyright (C) 2012-2013 Mihai Preda

#include "SymbolTable.h"
#include <stdio.h>

SymbolTable::SymbolTable(GC *gc) : 
    level(0)
{
    starts[0] = 0;
}

SymbolTable::~SymbolTable() {
}

int SymbolTable::pushContext() {
    // printf("SymbolTable push level %d\n", level+1);
    ++level;
    starts[level] = names.size();
    return level;
}

int SymbolTable::popContext() {
    names.setSize(starts[level]);
    slots.setSize(starts[level]);
    return --level;
}

int SymbolTable::getLevel(int pos) {
    for (int i = level; i >= 0; --i) {
        if (starts[i] <= pos) { return i; }
    }
    return -1;
}

int SymbolTable::findPos(Value name) {
    Value *buf = names.buf();
    int n = names.size();
    for (Value *p = buf + n - 1; p >= buf; --p) {
        if (equals(name, *p)) {
            return p - buf;
        }
    }
    return -1;
}

Value SymbolTable::get(Value name) {
    int pos = findPos(name);
    if (pos == -1) { return VNIL; }
    int slot = slots.get(pos);
    int lvl  = getLevel(pos);
    return VAL_REG((slot << 8) | lvl);
}

void SymbolTable::set(Value name, int slot, int level) {
    if (level == -1 || level == this->level) {
        names.push(name);
        slots.push(slot);
        return;
    } else {
        int pos = starts[level + 1];
        names.insertAt(pos, name);
        slots.insertAt(pos, slot);
        for (int i = level + 1; i <= this->level; ++i) {
            ++starts[i];
        }
    }
}
