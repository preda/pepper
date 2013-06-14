// Copyright (C) 2012-2013 Mihai Preda

#include "SymbolTable.h"
#include "GC.h"
#include "Object.h"
#include "Array.h"
#include "String.h"
#include <stdio.h>

SymbolTable::SymbolTable() {
    ((Object *) this)->setType(O_SYMTAB);
    enterBlock(true);
}

SymbolTable::~SymbolTable() {
    exitBlock(true);
}

void SymbolTable::traverse(GC *gc) {
    gc->markValVect(names.buf(), names.size());
}

void SymbolTable::add(GC *gc, Array *regs, const char *name, Value v) {
    regs->push(v);
    set(String::value(gc, name), regs->size() - 1);
}

int SymbolTable::localsTop() {
    return starts.top()->localsTop;
}

void SymbolTable::enterBlock(bool isProto) {
    if (isProto) {
        starts.push(BlockInfo(slots.size()));
        protos.push(starts.size() - 1);
    } else {
        starts.push(BlockInfo(slots.size(), starts.top()));
    }
}

void SymbolTable::exitBlock(bool isProto) {
    assert(isProto == (*protos.top() == starts.size() - 1));
    if (isProto) {
        protos.pop();
    }
    int sz = starts.pop().start;
    names.setSize(sz);
    slots.setSize(sz);
}

int SymbolTable::getProtoLevel(int pos) {
    for (int i = protos.size()-1; i >= 0; --i) {
        if (starts[protos[i]].start <= pos) { return i; }
    }
    return -1;
}

int SymbolTable::getBlockLevel(int pos) {
    for (int i = starts.size() - 1; i >= 0; --i) {
        if (starts[i].start <= pos) { return i; }
    }
    return -1;
}

int SymbolTable::findPos(Value name) {
    for (Value *buf = names.buf(), *p = buf + names.size() - 1; p >= buf; --p) {
        if (equals(name, *p)) {
            return p - buf;
        }
    }
    return -1;
}

bool SymbolTable::get(Value name, int *slot, int *protoLevel, int *blockLevel) {
    int pos = findPos(name);
    if (pos == -1) { return false; }
    if (slot) {
        *slot = slots.get(pos);
    }
    if (protoLevel) {
        *protoLevel = getProtoLevel(pos);
    }
    if (blockLevel) {
        *blockLevel = getBlockLevel(pos);
    }
    return true;
}

bool SymbolTable::definedInThisBlock(Value name) {
    int nameLevel = 0;
    int blockLevel = this->blockLevel();
    get(name, 0, 0, &nameLevel);    
    assert(blockLevel > 0 && nameLevel <= blockLevel);
    return nameLevel == blockLevel;
}

void SymbolTable::set(Value name, int slot) {
    names.push(name);
    slots.push(slot);
}

void SymbolTable::setUpval(Value name, int slot, int protoLevel) {
    assert(slot < 0);
    int block = protos.get(protoLevel);
    int pos = starts.get(block).start;
    names.insertAt(pos, name);
    slots.insertAt(pos, slot);
    for (BlockInfo *buf = starts.buf(), *p = buf + block + 1, *end = buf + starts.size(); p < end; ++p) { ++(p->start); }
}
