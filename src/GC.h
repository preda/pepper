// Copyright (C) 2012 Mihai Preda

#pragma once

#include "Value.h"

struct Object;
class VM;
template <typename T> struct Vector;

class GC {
    int size;
    long *map;
    int n;
    unsigned bytesSinceLast;
    Vector<Object *> *grayStack;

    void add(long v);
    void growMap();
    void traverse(Object *o);

 public:
    GC();
    ~GC();

    void maybeCollect(VM *vm, Value *vmStack, int vmStackSize) {
        static const unsigned kLimit = 256;
        if (bytesSinceLast >= kLimit) {
            collect(vm, vmStack, vmStackSize);
        }
    }

    void collect(VM *vm, Value *vmStack, int vmStackSize);

    Object *alloc(int type, int bytes, bool traversable);
    void mark(Object *p);
    void markValVect(Value *buf,   int size);
    void markObjVect(Object **buf, int size);
};
