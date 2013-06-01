// Copyright (C) 2012-2013 Mihai Preda

#pragma once

#include "Value.h"
#include "Vector.h"

struct Object;
class VM;
template <typename T> struct Vector;

class GC {
    int size;
    long *map;
    int n;
    unsigned bytesSinceLast;
    Vector<Object *> grayStack;
    Vector<Object *> roots;
    
    void add(long v);
    void growMap();
    void traverse(Object *o);

 public:
    const Value EMPTY_MAP, EMPTY_ARRAY;
    
    GC();
    ~GC();

    void addRoot(Object *obj);
    void clearRoots();

    void maybeCollect(VM *vm, Value *vmStack, int vmStackSize) {
        static const unsigned kLimit = 256;
        if (bytesSinceLast >= kLimit) {
            collect(vm, vmStack, vmStackSize);
        }
    }

    void collect(VM *vm, Value *vmStack, int vmStackSize);

    Object *alloc(int bytes, bool traversable);
    void mark(Object *p);
    void markValVect(Value *buf,   int size);
    void markObjVect(Object **buf, int size);
};
