// Copyright (C) 2012 Mihai Preda

#include "GC.h"
#include "Object.h"
#include "Vector.h"
#include "Array.h"
#include "Map.h"
#include "String.h"
#include "CFunc.h"
#include "Proto.h"
#include "VM.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

enum {BIT_MARK=1, BIT_TRAVERSABLE=2, };

GC::GC() :
    size(32),
    map((long *) calloc(size, sizeof(long))),
    n(0),
    nLastCollect(8),
    grayStack(0)
{
}

GC::~GC() {
    free(map);
    map = 0;
}

void GC::growMap() {
    size += size;
    long *oldMap = map;
    map = (long *) calloc(size, sizeof(long));
    for (long *p = oldMap, *end = p + (size>>1); p < end; ++p) {
        long v = *p;
        if (v) { add(v); }
    }
    free(oldMap);
}

void GC::add(long v) {
    const unsigned mask = size - 1;
    unsigned i = PTR_HASH(v) & mask;
    while (map[i]) { i = (i + 1) & mask; }
    map[i] = v;
}

void GC::mark(Object *o) {
    // fprintf(stderr, "mark %p\n", o);
    if (o) {
        const unsigned mask = size - 1;
        long p = (long) o;
        unsigned i = PTR_HASH(p) & mask;
        p |= 3;
        while ((map[i] | 3) != p && map[i]) { i = (i + 1) & mask; }
        assert(map[i]);
        if (!(map[i] & BIT_MARK)) {
            map[i] |= BIT_MARK;
            if (map[i] & BIT_TRAVERSABLE) {
                grayStack->push(o);
            }
        }
    }
}

void GC::markValVect(Value *p, int size) {
    for (Value *end = p + size; p < end; ++p) {
        const Value v = *p;
        if (IS_OBJ(v)) { mark(GET_OBJ(v)); }
    }
}

void GC::markObjVect(Object **p, int size) {
    for (Object **end = p + size; p < end; ++p) {
        mark(*p);
    }
}

Object *GC::alloc(int type, int bytes, bool traversable) {
    if (n >= (size >> 1)) { growMap(); }
    Object *p = (Object *) calloc(1, bytes);
    add((long)p | (traversable ? BIT_TRAVERSABLE : 0));
    ++n;
    // printf("alloc %d %p type %d %s\n", bytes, p, type, Object::getTypeName(type));
    return p;
}

#define DISPATCH(o) switch(o->type()) { \
 case O_ARRAY: ACTION(o, Array);  break;\
 case O_MAP:   ACTION(o, Map);    break;\
 case O_STR:   ACTION(o, String); break;\
 case O_FUNC:  ACTION(o, Func); break;\
 case O_CFUNC: ACTION(o, CFunc);  break;\
 case O_PROTO: ACTION(o, Proto); break;\
 default: ERR(true, E_OBJECT_TYPE);\
}

static void destruct(Object *o) {
    // printf("free %p type %d\n", o, o->type());
#define ACTION(o, type) ((type *)o)->~type()
    DISPATCH(o);
#undef ACTION
}


void GC::traverse(Object *o) {
#define ACTION(o, type) ((type *)o)->traverse(this)
    DISPATCH(o);
#undef ACTION
}

static void release(long p) {
    Object *obj = (Object *) (p & ~(long)(BIT_MARK | BIT_TRAVERSABLE));    
    destruct(obj);
    free(obj);
}

void GC::collect(VM *vm, Value *vmStack, int vmStackSize) {
    // printf("GC before %d %d\n", n, size);
    // fprintf(stderr, "collect %d %p\n", vmStackSize, GET_OBJ(vmStack[0]));
    int nInitial = n;
    {
        Vector<Object*> stack;
        grayStack = &stack;
        // vm->traverse();
        markValVect(vmStack, vmStackSize);
        while (stack.size()) {
            traverse(stack.pop());
        }
        grayStack = 0;
    }

    const unsigned mask = size - 1;
    for (long *p = map+size-1, *end = map-1; p != end; --p) {
        const long clearMark = ~(long)BIT_MARK;
        if (*p & BIT_MARK) {
            *p &= clearMark;
        } else if (*p) {
            release(*p);
            --n;
            *p = 0;
            long *hole = p;
            long *pp = map + (((hole-map) + 1) & mask);
            while (*pp) {
                long *t = map + (PTR_HASH(*pp) & mask);
                if (((hole - t) & mask) < ((pp - t) & mask)) {
                    if (pp < hole && !(*pp & BIT_MARK)) {
                        release(*pp);
                        --n;
                    } else {
                        *hole = *pp;
                    }
                    hole  = pp;
                    *hole = 0;
                }
                pp = map + (((pp-map) + 1) & mask);
            }            
        }
    }
    nLastCollect = n;
    printf("GC collected %d left %d table %d\n", (nInitial - n), n, size);
}
