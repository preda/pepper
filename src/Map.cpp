#include "Map.h"
#include "Array.h"
#include "String.h"
#include "Value.h"
#include "Object.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

Map::Map(const unsigned iniSize) {
    // printf("Map %p\n", this);
    size = 0;
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

#define INC(h) h = (h + 1) & mask
#define HASH(pos) (hashCode(keys[pos]) & mask)

Map *Map::alloc(Vector<Value> *keys, Vector<Value> *vals) {
    Map *m = alloc(keys->size);
    m->set(keys, vals);
    return m;
}

bool Map::equals(Map *o) {
    if (size != o->size) {
        return false;
    }
    return true; // TODO
}

void Map::set(Vector<Value> *keys, Vector<Value> *vals) {
    for (Value *pk=keys->buf, *end=pk+keys->size, *pv=vals->buf; pk < end; ++pk, ++pv) {
        set(*pk, *pv, true);
    }
}

Map *Map::copy() {
    Map *m = alloc(0);
    m->size = size;
    m->n = n;
    m->buf = (Value *) realloc(buf, n * 12);
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
        if (pos == EMPTY) { return NIL; }
        if (keys[pos] == key) { return keys[pos + (n>>1)]; }
        INC(h);
    }
}

bool Map::set(Value key, Value val, bool overwrite) {
    if (!val) {
        return remove(key);
    }
    if (size + size >= n) {
        grow();
    }
    const unsigned mask = n - 1;
    int * const map = (int *)(buf + n);
    Value * const keys = buf;
    unsigned h = hashCode(key) & mask;
    while (true) {
        const int pos = map[h];
        if (pos == EMPTY) {
            map[h] = size;
            keys[size]          = key;
            keys[size + (n>>1)] = val;
            ++size;
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
        case O_ARRAY: appendArray((Array *) v);
        case O_MAP:   appendMap((Map *) v);
        }
    }
}

void Map::appendMap(Map *m) {
    Value *pk = m->buf, *end = m->buf + m->size;
    Value *pv = pk + (m->n >> 1);
    while (pk < end) {
        set(*pk++, *pv++, false);
    }
}

void Map::appendArray(Array *a) {
    Value ONE = VAL_INT(1);
    for (Value *p = a->vect.buf, *end = a->vect.buf+a->vect.size; p < end; ++p) {
        set(*p, ONE, false);
    }
}

void Map::appendChars(char *s, int size) {
    Value c = String::makeVal(1);
    char *pc = GET_CSTR(c);
    for (char *end = s + size; s < end; ++s) {
        *pc = *s;
        set(c, VAL_INT(1), false);
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
    if (deletePos != size-1) {
        int newSize = size - 1;
        keys[deletePos] = keys[newSize];
        keys[deletePos + (n>>1)] = keys[newSize + (n>>1)];
        h = HASH(newSize);
        while (map[h] != newSize) { INC(h); }
        map[h] = deletePos;
    }
    --size;
    return true;
}

void Map::grow() {
    n += n;
    buf = (Value *) realloc(buf, n * 24);
    memset(buf + n, 0xff, n * 4);
    memcpy(buf + (n>>1), buf + (n>>2), size * 8);
    const unsigned mask = n - 1;
    int * const map = (int *)(buf + n);
    // Value * const keys = buf;
    int i = 0;
    for (Value *p = buf, *end = buf+size; p < end; ++p, ++i) {
        unsigned h = hashCode(*p) & mask;
        while (map[h] != EMPTY) { INC(h); }
        map[h] = i;
    }
}
