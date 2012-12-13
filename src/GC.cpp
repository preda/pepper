#include "GC.h"
#include "Object.h"
#include "Vector.h"
#include "Array.h"
#include "Map.h"
#include "String.h"
#include "CFunc.h"
#include "Proto.h"

#include <stdlib.h>
#include <string.h>

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
    while ((map[i] | 3) != (p | 3)) {
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
        Value v = *p;
        if (IS_OBJ(v)) { _mark(GET_OBJ(v)); }
    }
}

void GC::_markVectorObj(Object **p, int size) {
    for (Object **end = p + size; p < end; ++p) {
        _mark(*p);
    }
}

Object *GC::_alloc(int type, int bytes, bool traversable) {
    if (nPtr >= (1 << (nBits-1))) {
        growMap();
    }
    Object *p = (Object *) malloc(bytes);
    memset(p, 0, bytes);
    // p->setType(type);
    add((unsigned long)p | (traversable ? BIT_TRAVERSABLE : 0));
    ++nPtr;
    return p;
}

#define DISPATCH(o) switch(o->type()) { \
 case O_ARRAY: ACTION(o, Array);  break;\
 case O_MAP:   ACTION(o, Map);    break;\
 case O_STR:   ACTION(o, String); break;\
 case O_CFUNC: ACTION(o, CFunc);  break;\
 default: ERR(true, E_OBJECT_TYPE);\
}

static void destruct(Object *o) {
#define ACTION(o, type) ((type *)o)->~type()
    DISPATCH(o);
#undef ACTION
}


static void traverse(Object *o) {
#define ACTION(o, type) ((type *)o)->traverse()
    DISPATCH(o);
#undef ACTION
}

void GC::_markAndSweep(Object *root) {
    {
        Vector<Object*> stack(1024);
        grayStack = &stack;
        stack.push(root);
        while (stack.size()) {
            traverse(stack.pop());
        }
        grayStack = 0;
    }
    
    for (unsigned long *p = map, *end = map + (1<<nBits); p < end; ++p) {
        if (*p && !(*p & BIT_MARK)) {
            Object *obj = (Object *) *p;
            *p = -1L;
            destruct(obj);
            free(obj);
        }
    }
}
