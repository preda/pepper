#include "GC.h"
#include "Object.h"
#include "Vector.h"
#include <stdlib.h>

#define EMPTY_MARK ((unsigned long)-1L)

GC *GC::gc = new GC();

GC::GC() {
    nBits = 0;
    map   = 0;
    nPtr  = 0;
    growMap();
}

GC::~GC() {
    free(map);
    map = 0;
}

void GC::growMap() {
    nBits = nBits ? nBits+1 : 10;
    unsigned long *oldMap = map;
    map = (unsigned long *) calloc(1<<nBits, sizeof(long));
    if (oldMap) {
        for (unsigned long *p = oldMap, *end = oldMap + (1<<(nBits-1)); p < end; ++p) {
            unsigned long v = *p;
            if (v && v != EMPTY_MARK) {
                add(v);
            }
        }
        free(oldMap);
    }
}

void GC::add(unsigned long v) {
    const unsigned mask = (1<<nBits) - 1;
    unsigned i = PTR_HASH(v) & mask;
    unsigned step = 1;
    unsigned long m;
    while ((m=map[i]) && m != EMPTY_MARK) {
        i = (i + step++) & mask;
    }
    map[i] = v;
}

void GC::_mark(Object *o) {
    unsigned long p = (long) o;
    const unsigned mask = (1<<nBits) - 1;
    unsigned i = PTR_HASH(p) & mask;
    unsigned step = 1;
    while (map[i] | 3 != p | 3) {
        i = (i + step++) & mask;
    }
    if (!(map[i] & BIT_MARK)) {
        map[i] |= BIT_MARK;
        if (map[i] & BIT_TRAVERSABLE) {
            grayStack->push(o);
        }
    }
}

void GC::_markVector(Value *p, int size) {
    for (Value *end = p + size; p < end; ++p) {
        if (TAG(*p) == OBJECT && *p) {
            _mark((Object *) *p);
        }
    }
}

Object *GC::_alloc(int type, int bytes, bool traversable) {
    if (nPtr >= (1 << (nBits-1))) {
        growMap();
    }
    Object *p = (Object *) malloc(bytes);
    p->type = type;
    add((unsigned long)p | (traversable ? BIT_TRAVERSABLE : 0));
    ++nPtr;
    return p;
}

void GC::_markAndSweep(Object *root) {
    {
        Vector<Object*> stack(1024);
        grayStack = &stack;
        stack.push(root);
        while (stack.size) {
            stack.pop()->traverse();
        }
        grayStack = 0;
    }
    
    for (unsigned long *p = map, *end = map + (1<<nBits); p < end; ++p) {
        if (*p && !(*p & BIT_MARK)) {
            Object *obj = (Object *) *p;
            *p = -1L;
            obj->destroy();
            free(obj);
        }
    }
}
