// Copyright (C) 2012 Mihai Preda

#pragma once

#include "Value.h"

struct Object;
template <typename T> struct Vector;

class GC {
    static const unsigned BIT_MARK=1, BIT_TRAVERSABLE=2;
    static GC *gc;

    int nBits;
    unsigned long *map;
    int nPtr;
    Vector<Object *> *grayStack;

    void add(unsigned long v);
    void growMap();
    void traverse(Object *o);

 public:
    GC();
    ~GC();

    Object *alloc(int type, int bytes, bool traversable);
    void mark(Object *p);
    void markAndSweep(Object *root);
    void markVector(Value *buf, int size);
    void markVectorObj(Object **buf, int size);


    /*
    static Object *alloc(int type, int bytes, bool traversable) {
        // printf("GC::alloc %d %d %d\n", type, bytes, traversable);
        return gc->_alloc(type, bytes, traversable);
    }

    static void mark(Object *p) {
        gc->_mark(p);
    }

    static void markAndSweep(Object *root) {
        gc->_markAndSweep(root);
    }

    static void markVector(Value *buf, int size) {
        gc->_markVector(buf, size);
    }

    static void markVectorObj(Object **buf, int size) {
        gc->_markVectorObj(buf, size);
    }
    */
};
