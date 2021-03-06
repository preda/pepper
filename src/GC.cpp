// Copyright (C) 2012-2013 Mihai Preda

#include "GC.h"
#include "Object.h"
#include "Vector.h"
#include "Array.h"
#include "Map.h"
#include "String.h"
#include "CFunc.h"
#include "Proto.h"
#include "SymbolTable.h"
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
    bytesSinceLast(0),
    EMPTY_MAP(VAL_OBJ(Map::alloc(this))),
    EMPTY_ARRAY(VAL_OBJ(Array::alloc(this)))
{
    addRoot(GET_OBJ(EMPTY_MAP));
    addRoot(GET_OBJ(EMPTY_ARRAY));
}

GC::~GC() {
    clearRoots();
    collect(0, 0);
    
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
                grayStack.push(o);
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

Object *GC::alloc(int bytes, bool traversable) {
    assert(bytes > 0);
    if (n >= (size >> 1)) { growMap(); }
    Object *p = (Object *) calloc(1, bytes);
    add((long)p | (traversable ? BIT_TRAVERSABLE : 0));
    ++n;
    bytesSinceLast += bytes;
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
 case O_SYMTAB: ACTION(o, SymbolTable); break;\
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

void GC::addRoot(Object *obj) {
    roots.push(obj);
}

void GC::clearRoots() {
    roots.clear();
}

void GC::collect(Value *vmStack, int vmStackSize) {
    // printf("GC before %d %d\n", n, size);
    // fprintf(stderr, "collect %d %p\n", vmStackSize, GET_OBJ(vmStack[0]));
    assert(grayStack.size() == 0);
    markObjVect(roots.buf(), roots.size());
    markValVect(vmStack, vmStackSize);
    while (grayStack.size()) {
        traverse(grayStack.pop());
    }
    assert(grayStack.size() == 0);

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
    // printf("GC collected %d left %d table %d bytesSince %d\n", (nInitial - n), n, size, bytesSinceLast);
    bytesSinceLast = 0;
}
