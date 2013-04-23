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
#include <stdarg.h>

void Map::traverse(GC *gc) {
    int sz = size();
    gc->markValVect(valBuf(), sz);
    gc->markValVect(keyBuf(), sz);
}

Map::Map() :
    hasGet(false),
    hasSet(false)
{
    ((Object *) this)->setType(O_MAP);
}

Map::~Map() {
}

Value Map::makeMap(GC *gc, ...) {
    va_list ap;
    va_start(ap, gc);
    Map *m = Map::alloc(gc);
    while (true) {
        char *name = va_arg(ap, char*);
        if (!name) { break; }
        // fprintf(stderr, "%s\n", name);
        Value v = va_arg(ap, Value);
        m->rawSet(String::value(gc, name), v);
    }
    va_end(ap);
    return VAL_OBJ(m);
}

bool Map::equals(Map *o) {
    const int sz = size();
    if (sz != o->size()) { return false; }
    for (Value *pk = keyBuf(), *end=pk+sz, *pv=valBuf(); pk < end; ++pk, ++pv) {
        if (!::equals(*pv, o->rawGet(*pk))) { return false; }
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
    m->set(keyBuf(), valBuf(), size());
    return m;
}

Value Map::rawSet(Value key, Value val) {
    return IS_NIL(val) ? map.remove(key) : map.set(key, val);
}

Value Map::set(Value key, Value val, bool *again) {
    *again = false;
    if (key == String::__SET) {
        hasSet = IS_FUNC(val) || IS_CFUNC(val) || IS_MAP(val);
        return rawSet(key, val);
    } else if (!hasSet || map.contains(key)) {
        if (key == String::__GET) {
            hasGet = IS_FUNC(val) || IS_CFUNC(val) || IS_MAP(val);
        }
        return rawSet(key, val);
    } else {
        *again = true;
        return rawGet(String::__SET);
    }
}

Value Map::get(Value key, bool *recurse) {
    Value v = rawGet(key);
    const bool rec = IS_NIL(v) && hasGet;
    *recurse = rec;
    return rec ? rawGet(String::__GET) : v;
}

void Map::add(Value v) {
    ERR(!(IS_MAP(v)), E_ADD_NOT_COLLECTION);
    Map *m = MAP(v);
    set(m->keyBuf(), m->valBuf(), m->size());
}
