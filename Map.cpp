#include "Map.h"
#include "Object.h"
#include "Array.h"
#include "GC.h"
#include "Value.h"
#include <new>
#include <stdio.h>

#define INC(h) h = (h + 1) & mask
#define HASH(pos) (hashCode(keys[pos]) & mask)

Map *Map::alloc(int iniSize) {
    return new (GC::alloc(MAP, sizeof(Map), true)) Map(iniSize);
}

Map::Map(int iniSize) {
    printf("Map %p\n", this);
    size = 0;
    n = 8;
    while (iniSize > (n>>1)) {
        n += n;
    }
    buf = (Value *) malloc(n * 12);
    memset(buf + n, 0xff, n * 4); 
}

void Map::destroy() {
    free(buf);
    buf = 0;
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

void Map::appendString(char *s, int size) {
    Value c = VALUE(STRING+1, 0);
    Value ONE = VAL_INT(1);
    for (char *end = s + size; s < end; ++s) {
        *((char *) &c) = *s;
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
    const int deletePos = map[h];
    map[h] = EMPTY;
    int hole = h;
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
    Value * const keys = buf;
    int i = 0;
    for (Value *p = buf, *end = buf+size; p < end; ++p, ++i) {
        unsigned h = hashCode(*p) & mask;
        while (map[h] != EMPTY) { INC(h); }
        map[h] = i;
    }
}
