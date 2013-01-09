// Copyright (C) 2012 - 2013 Mihai Preda

#include "Index.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

Index::Index() :
    allocSize(2),
    _size(0)    
{
    buf = (Value *) calloc(allocSize << 1, sizeof(Value));
}

Index::~Index() {
    free(buf);
    buf = 0;
    allocSize = _size = 0;
}

inline Value Index::getVal(int pos) {
    assert(pos >= 0 && pos < _size);
    return buf[pos];
}

void Index::remap() {
    buf = (Value *) realloc(buf, (allocSize << 1) * sizeof(Value));
    memset(buf + allocSize, 0xff, allocSize * sizeof(Value));
    const unsigned mask = (allocSize << 1) - 1;
    int *map = (int *) (buf + allocSize);
    for (Value *p = buf, *end = buf + _size; p < end; ++p) {
        unsigned h = hashCode(*p) & mask;
        while (map[h] != EMPTY) { h = (h + 1) & mask; }
        map[h] = p - buf;
    }
}

void Index::grow() {
    allocSize += allocSize;
    remap();
}

void Index::shrink() {
    if (allocSize > 2) {
        allocSize >>= 1;
        remap();        
    }
}

int Index::add(Value v) {
    if (_size >= allocSize) { grow(); }
    assert(_size < allocSize);
    int *map = (int *) (buf + allocSize);
    const unsigned mask = (allocSize << 1) - 1;
    unsigned h = hashCode(v) & mask;    
    while (true) {
        const int pos = map[h];
        if (pos == EMPTY) {
            map[h] = _size;
            buf[_size++] = v;            
            return -1;
        } else if (equals(getVal(pos), v)) {
            return pos;
        }
        h = (h + 1) & mask;
    }
}

int Index::remove(Value v) {
    int *map = (int *) (buf + allocSize);
    const unsigned mask = (allocSize << 1) - 1;
    unsigned h = hashCode(v) & mask;    
    int pos;
    while (true) {
        pos = map[h];
        if (pos == EMPTY) { return -1; }
        if (equals(getVal(pos), v)) { break; }
        h = (h + 1) & mask;
    }
    int deletePos = pos;
    buf[pos] = VNIL;
    map[h] = EMPTY;
    unsigned hole = h;
    while (true) {
        h = (h + 1) & mask;
        pos = map[h];
        if (pos == EMPTY) { break; }
        unsigned hh = hashCode(getVal(pos)) & mask;
        if ((hole < h && (hh <= hole || hh > h)) ||
            (hole > h && (hh <= hole && hh > h))) {
            map[hole] = pos;
            map[h]    = EMPTY;
            hole = h;
        }
    }
    // assert(deletePos >= 0 && deletePos < size);
    if (deletePos != _size - 1) {
        int s = _size - 1;
        Value v = buf[s];
        buf[deletePos] = v;
        h = hashCode(v) & mask;
        while (map[h] != EMPTY && map[h] != s) {
            h = (h + 1) & mask;
        }
        assert(map[h] == s);
        map[h] = deletePos;
    }
    --_size;
    if (_size < (allocSize >> 2)) {
        shrink();
    }
    return deletePos;
}

int Index::getPos(Value key) {
    const unsigned mask = (allocSize << 1) - 1;
    int *map = (int *) (buf + allocSize);
    unsigned h = hashCode(key) & mask;
    while (true) {
        int pos = map[h];
        if (pos == EMPTY) { return -1; }
        if (::equals(getVal(pos), key)) { return pos; }
        h = (h + 1) & mask;
    }
}
