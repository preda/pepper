// Copyright (C) 2012 Mihai Preda

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
    gc->markValVect(buf, size());
    gc->markValVect(buf+(n>>1), size());
}

Map::Map(const unsigned iniSize) {
    ((Object *) this)->setType(O_MAP);
    setSize(iniSize); 
    n = 8;
    while ((iniSize << 1) > n) {
        n += n;
    }
    buf = (Value *) malloc(n * 12);
    memset(buf + n, 0xff, n * 4); 
}

Map::~Map() {
    free(buf);
    buf = 0;
}

Value Map::value(GC *gc, unsigned n, NameValue *p) {
    Map *m = Map::alloc(gc, n);
    for (NameValue *end = p + n; p < end; ++p) {
        m->set(String::value(gc, p->name), p->value);
    }
    return VAL_OBJ(m);
}

#define INC(h) h = (h + 1) & mask
#define HASH(pos) (hashCode(keys[pos]) & mask)

Map *Map::alloc(GC *gc, Vector<Value> *keys, Vector<Value> *vals) {
    Map *m = alloc(gc, keys->size());
    m->set(keys, vals);
    return m;
}

bool Map::equals(Map *o) {
    if (size() != o->size()) {
        return false;
    }
    return true; // TODO
}

void Map::set(Vector<Value> *keys, Vector<Value> *vals) {
    for (Value *pk=keys->buf(), *end=pk+keys->size(), *pv=vals->buf(); pk < end; ++pk, ++pv) {
        set(*pk, *pv, true);
    }
}

Map *Map::copy(GC *gc) {
    Map *m = alloc(gc, size());
    assert(m->n <= n);
    while (m->n < n) { grow(); }
    memcpy(m->buf, buf, n * 12);
    return m;
}

Value Map::get(Value key) {
    const unsigned mask = n - 1;
    int * const map = (int *)(buf + n);
    Value * const keys = buf;
    unsigned h = hashCode(key) & mask;
    while (true) {
        const int pos = map[h];
        if (pos == EMPTY) { return VNIL; }
        if (::equals(keys[pos], key)) { return keys[pos + (n>>1)]; }
        INC(h);
    }
}

bool Map::set(Value key, Value val, bool overwrite) {
    if (IS_NIL(val)) {
        return remove(key);
    }
    unsigned s = size();
    if ((s << 1) >= n) {
        grow();
    }
    const unsigned mask = n - 1;
    int * const map = (int *)(buf + n);
    Value * const keys = buf;
    unsigned h = hashCode(key) & mask;
    while (true) {
        const int pos = map[h];
        if (pos == EMPTY) {
            map[h] = s;
            keys[s]          = key;
            keys[s + (n>>1)] = val;
            incSize();
            return false;
        }
        if (keys[pos] == key) { 
            if (overwrite) {
                keys[pos + (n>>1)] = val;
            }
            return true;
        }
        INC(h);
    }
}

void Map::add(Value v) {
    ERR(!(IS_ARRAY(v) || IS_STRING(v) || IS_MAP(v)), E_ADD_NOT_COLLECTION);
    if (IS_STRING(v)) {
        appendChars(GET_CSTR(v), len(v));
    } else {
        switch (O_TYPE(v)) {
        case O_ARRAY: appendArray(ARRAY(v));
        case O_MAP:   appendMap(MAP(v));
        }
    }
}

void Map::appendMap(Map *m) {
    Value *pk = m->buf, *end = m->buf + m->size();
    Value *pv = pk + (m->n >> 1);
    while (pk < end) {
        set(*pk++, *pv++, false);
    }
}

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

bool Map::remove(Value key) {
    const unsigned mask = n - 1;
    int * const map = (int *)(buf + n);
    Value * const keys = buf;
    unsigned h = hashCode(key) & mask;
    while (true) {
        const int pos = map[h];
        if (pos == EMPTY) { return false; }
        if (keys[pos] == key) { break; }
        INC(h);
    }
    const unsigned deletePos = map[h];
    map[h] = EMPTY;
    unsigned hole = h;
    while (true) {
        INC(h);
        const int pos = map[h];
        if (pos == EMPTY) { break; }
        const unsigned hh = HASH(pos);
        if ((hole < h && (hh <= hole || hh > h)) ||
            (hole > h && (hh <= hole && hh > h))) {
            map[hole] = pos;
            map[h]    = EMPTY;
            hole = h;
        }
    }
    unsigned s = size();
    if (deletePos != s - 1) {
        int newSize = s - 1;
        keys[deletePos] = keys[newSize];
        keys[deletePos + (n>>1)] = keys[newSize + (n>>1)];
        h = HASH(newSize);
        while (map[h] != newSize) { INC(h); }
        map[h] = deletePos;
    }
    _size -= (1<<4); //    --size;
    return true;
}

void Map::grow() {
    n += n;
    buf = (Value *) realloc(buf, n * 24);
    memset(buf + n, 0xff, n * 4);
    memcpy(buf + (n>>1), buf + (n>>2), size() * 8);
    const unsigned mask = n - 1;
    int * const map = (int *)(buf + n);
    // Value * const keys = buf;
    int i = 0;
    for (Value *p = buf, *end = buf+size(); p < end; ++p, ++i) {
        unsigned h = hashCode(*p) & mask;
        while (map[h] != EMPTY) { INC(h); }
        map[h] = i;
    }
}
