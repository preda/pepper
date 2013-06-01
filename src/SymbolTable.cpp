// Copyright (C) 2012-2013 Mihai Preda

#include "SymbolTable.h"
#include <stdio.h>

SymbolTable::SymbolTable() {
    enterBlock(true);
}

SymbolTable::~SymbolTable() {
    exitBlock(true);
}

void SymbolTable::enterBlock(bool isProto) {
    starts.push(slots.size());
    if (isProto) {
        protos.push(starts.size() - 1);
    }
}

void SymbolTable::exitBlock(bool isProto) {
    assert(isProto == (*protos.top() == starts.size() - 1));
    if (isProto) {
        protos.pop();
    }
    int sz = starts.pop();
    names.vect.setSize(sz);
    slots.setSize(sz);
}

int SymbolTable::getLevel(int pos) {
    for (int i = protos.size()-1; i >= 0; --i) {
        if (starts[protos[i]] <= pos) { return i; }
    }
    return -1;
}

int SymbolTable::findPos(Value name) {
    for (Value *buf = names.vect.buf(), *p = buf + names.vect.size() - 1; p >= buf; --p) {
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
    names.vect.push(name);
    slots.push(slot);
}

void SymbolTable::setUpval(Value name, int slot, int level) {
    assert(slot < 0);
    int block = protos.get(level);
    int pos = starts.get(block);
    names.vect.insertAt(pos, name);
    slots.insertAt(pos, slot);
    for (int *buf = starts.buf(), *p = buf + block + 1, *end = buf + starts.size(); p < end; ++p) { ++*p; }
}
