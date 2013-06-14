// Copyright (C) 2012-2013 Mihai Preda

#pragma once

#include "Vector.h"
#include "value.h"

class GC;
class Array;

struct BlockInfo {
BlockInfo(int start) :
    start(start), localsTop(0), nConsts(0) {}
    
BlockInfo(int start, BlockInfo *base) :
    start(start), localsTop(base->localsTop), nConsts(base->nConsts) {}
    
    int start;
    int localsTop;
    int nConsts;
};

// Object
class SymbolTable {
    Vector<Value> names;
    Vector<int> protos;
    Vector<BlockInfo> starts;
    Vector<int> slots;

    SymbolTable();
    int getProtoLevel(int pos);
    int getBlockLevel(int pos);
    int findPos(Value name);

 public:
    static SymbolTable *alloc(GC *gc);
    ~SymbolTable();

    void traverse(GC *gc); // GC
    void enterBlock(bool isProto);
    void exitBlock(bool isProto);
    
    bool get(Value name, int *slot, int *protoLevel, int *blockLevel);
    void set(Value name, int slot);
    void setUpval(Value name, int slot, int protoLevel);
    int protoLevel() { return protos.size() - 1; }
    int blockLevel() { return starts.size() - 1; }
    bool definedInThisBlock(Value name);
    int localsTop();
    
    void add(GC *gc, Array *regs, const char *name, Value v);
};
