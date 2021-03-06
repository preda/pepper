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
    int maxLocalsTop;
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
    int exitBlock(bool isProto); // returns maxLocalsTop
    
    bool get(Value name, int *slot, int *protoLevel, int *blockLevel);
    void set(Value name, int slot);
    int set(Value name);
    void setUpval(Value name, int slot, int protoLevel);
    int protoLevel() { return protos.size() - 1; }
    int blockLevel() { return starts.size() - 1; }
    bool definedInThisBlock(Value name);
    int localsTop();
    void addLocalsTop(int delta);
    
    void add(GC *gc, Array *regs, const char *name, Value v);
};
