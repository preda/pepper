// Copyright (C) 2012 - 2013 Mihai Preda

#include "Map.h"
#include "Array.h"
#include "String.h"
#include "Value.h"
#include "Object.h"
#include "GC.h"
#include "NameValue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void Map::traverse(GC *gc) {
    int sz = size();
    gc->markValVect(map.valBuf(), sz);
    gc->markValVect(map.keyBuf(), sz);
}

Map::Map() {
    ((Object *) this)->setType(O_MAP);
}

Map::~Map() {
}

Value Map::value(GC *gc, unsigned n, NameValue *p) {
    Map *m = Map::alloc(gc, n);
    for (NameValue *end = p + n; p < end; ++p) {
        m->set(String::value(gc, p->name), p->value);
    }
    return VAL_OBJ(m);
}

/*
Map *Map::alloc(GC *gc, Vector<Value> *keys, Vector<Value> *vals) {
    Map *m = alloc(gc, keys->size());
    m->set(keys, vals);
    return m;
}
*/

bool Map::equals(Map *o) {
    const int sz = size();
    if (sz != o->size()) { return false; }
    for (Value *pk = map.keyBuf(), *end=pk+sz, *pv=map.valBuf(); pk < end; ++pk, ++pv) {
        if (!::equals(*pv, o->get(*pk))) { return false; }
    }
    return true;
}

void Map::set(Vector<Value> *keys, Vector<Value> *vals) {
    const int sz = keys->size();
    assert(vals->size() == sz);
    set(keys->buf(), vals->buf(), sz);
}

void Map::set(Value *keys, Value *vals, int size) {
    for (Value *pk=keys, *end=keys+size, *pv=vals; pk < end; ++pk, ++pv) {
        map.set(*pk, *pv);
    }
}

Map *Map::copy(GC *gc) {
    Map *m = alloc(gc, size());
    m->set(map.keyBuf(), map.valBuf(), size());
    return m;
}

Value Map::set(Value key, Value val) {
    return IS_NIL(val) ? map.remove(key) : map.set(key, val);
}

void Map::add(Value v) {
    ERR(!(IS_MAP(v)), E_ADD_NOT_COLLECTION);
    Map *m = MAP(v);
    set(m->map.keyBuf(), m->map.valBuf(), m->size());
}

/*
void Map::appendArray(Array *a) {
    for (Value *p = a->buf(), *end = p+a->size(); p < end; ++p) {
        set(*p, ONE, false);
    }
}

void Map::appendChars(char *s, int size) {
    Value c = String::value(0, 1);
    char *pc = GET_CSTR(c);
    for (char *end = s + size; s < end; ++s) {
        *pc = *s;
        set(c, ONE, false);
    }
}
*/
