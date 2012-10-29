#pragma once

#include "Value.h"
#include <stdio.h>

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

    Object *_alloc(int type, int bytes, bool traversable);
    void _mark(Object *p);
    void _markAndSweep(Object *root);
    void _markVector(Value *buf, int size);

 public:
    GC();
    ~GC();

    static Object *alloc(int type, int bytes, bool traversable) {
        printf("GC::alloc %d %d %d\n", type, bytes, traversable);
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
};
