// Copyright (C) 2012-2013 Mihai Preda

#include "SymbolTable.h"
#include <stdio.h>

SymbolTable::SymbolTable(GC *gc) : 
    curLevel(0)
{
    starts[0] = 0;
}

SymbolTable::~SymbolTable() {
}

int SymbolTable::pushContext() {
    ++curLevel;
    starts[curLevel] = names.size();
    return curLevel;
}

int SymbolTable::popContext() {
    names.setSize(starts[curLevel]);
    slots.setSize(starts[curLevel]);
    return --curLevel;
}

int SymbolTable::getLevel(int pos) {
    for (int i = curLevel; i >= 0; --i) {
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
    if (level == curLevel) {
        set(name, slot);
    } else {
        assert(level < curLevel);
        int pos = starts[level + 1];
        names.insertAt(pos, name);
        slots.insertAt(pos, slot);
        for (int i = level + 1; i <= curLevel; ++i) {
            ++starts[i];
        }
    }
}
