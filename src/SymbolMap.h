#pragma once

#include "common.h"

enum Kind {
    KIND_EMPTY  = 0,
    KIND_BRIDGE = 1,
    KIND_REGUP   = 2, // local if slot >=0, upval otherwise
    //     KIND_UPVAL   = 3,
    KIND_KEYWORD = 4,
};

struct SymbolData {
    byte kind;
    byte level;
    short slot;

    SymbolData(int k, int l, int s) : kind(k), level(l), slot(s) {}

    bool isEmpty() {
        return kind==0 || kind==1;
    }
};

struct Symbol {
    u64 key;
    SymbolData d;
};

struct HashEntry {
    unsigned key;
    SymbolData d;
};

class SymbolMap {
    int allocSize;
    int size;
    HashEntry *tab;

 public:
    SymbolMap();
    ~SymbolMap();

    bool del(u64 key) {
        HashEntry *e = get(key);
        if (!e) { return false; }
        if (e < tab + (allocSize - 1) && (e+1)->d.kind==KIND_EMPTY) {
            e->d.kind = KIND_EMPTY;
            --size;
        } else {
            e->d.kind = KIND_BRIDGE;
        }
        return true;
    }

    HashEntry *get(u64 key);
    HashEntry *add(u64 key);
};
