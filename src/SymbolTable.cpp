// Copyright (C) 2012-2013 Mihai Preda

#include "SymbolTable.h"
#include <stdio.h>

SymbolTable::SymbolTable(GC *gc) : 
    topLevel(0)
{
    starts[0] = 0;
}

SymbolTable::~SymbolTable() {
}

int SymbolTable::pushContext() {
    ++topLevel;
    starts[topLevel] = names.size();
    return topLevel;
}

int SymbolTable::popContext() {
    names.setSize(starts[topLevel]);
    slots.setSize(starts[topLevel]);
    return --topLevel;
}

int SymbolTable::getLevel(int pos) {
    for (int i = topLevel; i >= 0; --i) {
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
    int level  = getLevel(pos);
    return VAL_REG((slot << 8) | level);
}

void SymbolTable::set(Value name, int slot) {
    names.push(name);
    slots.push(slot);
}

void SymbolTable::set(Value name, int slot, int level) {
    if (level == topLevel) {
        set(name, slot);
    } else {
        assert(level < topLevel);
        int pos = starts[level + 1];
        names.insertAt(pos, name);
        slots.insertAt(pos, slot);
        for (int i = level + 1; i <= topLevel; ++i) {
            ++starts[i];
        }
    }
}
