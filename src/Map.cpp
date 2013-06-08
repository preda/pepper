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

Map::Map() {
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

Value Map::keysField(VM *vm, int op, void *data, Value *stack, int nArgs) {
    return VNIL;
}

bool Map::equals(Map *o) {
    const int sz = size();
    if (sz != o->size()) { return false; }
    for (Value *pk = keyBuf(), *end=pk+sz, *pv=valBuf(); pk < end; ++pk, ++pv) {
        if (!::equals(*pv, o->rawGet(*pk))) { return false; }
    }
    return true;
}

Value Map::remove(Value key) {
    const int pos = index.remove(key);
    if (pos < 0) { 
        return VNIL; 
    } else {
        Value val = vals.get(pos);
        Value top = vals.pop();
        if (pos < index.size()) {
            vals.setDirect(pos, top);
        }
        return val;
    }
}

/*
Value Map::getOrAdd(Value key, Value v) {
    const int pos = index.add(key);
    return pos >= 0 ? vals.get(pos) : (vals.push(v), v);
}
*/

void Map::setVectors(Vector<Value> *keys, Vector<Value> *vals) {
    const int sz = keys->size();
    assert(vals->size() == sz);
    setArrays(keys->buf(), vals->buf(), sz);
}

void Map::setArrays(Value *keys, Value *vals, int size) {
    for (Value *pk=keys, *end=keys+size, *pv=vals; pk < end; ++pk, ++pv) {
        set(*pk, *pv);
    }
}

Map *Map::copy(GC *gc) {
    Map *m = alloc(gc, size());
    m->setArrays(keyBuf(), valBuf(), size());
    return m;
}

Value Map::rawSet(Value key, Value v) {
    const int pos = index.add(key);
    Value prev = VNIL;
    if (pos >= 0) {
        prev = vals.get(pos);
        vals.setDirect(pos, v);
    } else {
        vals.push(v);
    }
    return prev;
}

Value Map::set(Value key, Value v) {
    /*
    int pos = getPos(key);
    if (pos == -1) { // key not found
        if (IS_NIL(v)) {
            return VNIL;
        } else {
            return rawSet(key, v);
        }
    } else {
        Value old = vals.get(pos);
        if (IS_ARRAY(old)) {
            Array *a = ARRAY(old);
            if (a->size() > 0 && a->getI(0) == STATIC_STRING("_prop")) {
                return old; // no set
            }
        }
    }
    */
    return IS_NIL(v) ? remove(key) : rawSet(key, v);
}

Value Map::rawGet(Value key) {
    const int pos = index.getPos(key);
    return pos < 0 ? VNIL : vals.get(pos);
}

Value Map::get(Value key) {
    Value v = rawGet(key);
    if (IS_NIL(v)) {
        Value super = rawGet(STATIC_STRING("super"));
        if (IS_MAP(super)) {
            v = MAP(super)->get(key);
        }
    }
    return v;
}

void Map::add(Value v) {
    ERR(!(IS_MAP(v)), E_ADD_NOT_COLLECTION);
    Map *m = MAP(v);
    setArrays(m->keyBuf(), m->valBuf(), m->size());
}
