#pragma once

#include "Vector.h"
#include "common.h"
#include "fnv.h"

enum Kind {
    KIND_EMPTY  = 0,
    KIND_BRIDGE = 1,
    KIND_LOCAL  = 2,
    KIND_UPVAL  = 3,
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

#define INC(p) p = (p + 1) & mask

class SymbolMap {
    int allocSize;
    int size;
    HashEntry *tab;

 public:
    SymbolMap() {
        size = 0;
        allocSize = 32;
        tab = (HashEntry *) calloc(allocSize, sizeof(HashEntry));
    }

    ~SymbolMap() {
        free(tab);
        tab = 0;
    }

    bool del(u64 key) {
        HashEntry *e = get(key);
        if (!e) { return false; }
        if (e < tab + (allocSize - 1) && (e+1)->kind==KIND_EMPTY) {
            e->d.kind = KIND_EMPTY;
            --size;
        } else {
            e->d.kind = KIND_BRIDGE;
        }
        return true;
    }

    HashEntry *get(u64 key) {
        const unsigned mask = allocSize - 1;
        const unsigned hig = key >> 32;
        const unsigned low = (unsigned) key;
        unsigned pos  = hig & mask;
        const unsigned h = hig ^ low;
        while (true) {
            HashEntry *e = tab + pos;
            const unsigned ekey = e->key;
            if (ekey == h && !e->d.isEmpty()) {
                return e;
            }
            if (!ekey && !e->d.kind) {
                    return 0;
            }
            INC(pos);
        }
    }

    HashEntry *add(u64 key) {
        const unsigned mask = allocSize - 1;
        const unsigned hig = key >> 32;
        const unsigned low = (unsigned) key;
        unsigned pos  = hig & mask;
        const unsigned h = hig ^ low;
        while (true) {
            HashEntry *e = tab + pos;
            const unsigned ekey = e->key;
            if (ekey == h) {
                return e;
            }
            if (!ekey && e->d.isEmpty()) {
                e->key = h;
                return e;
            }
            INC(pos);
        }
    }

};

typedef Vector<Symbol> SymbolVect;

class SymbolTable {
    int level;
    SymbolVect undoLog[16];
    SymbolMap map;

    void undo(Symbol *s) {
        u64 key = s->key;
        if (s->d.isEmpty()) {
            map.del(key);
        } else {
            HashEntry *e = map.get(key);
            e->d = s->d;
        }
    }

    void set(SymbolVect *currUndo, u64 key, SymbolData data) {
        Symbol *s = currUndo->push();
        s->key    = key;
        HashEntry *e = map.add(key);
        s->d = e->d;
        e->d = data;
    }
    
 public:
    SymbolTable() {
        level = 0;
    }
   
    int pushContext() {
        ++level;
    }

    int popContext() {
        SymbolVect *currUndo = undoLog + level;
        int n = currUndo->size;
        Symbol *buf = currUndo->buf;
        for (Symbol *s = buf + n - 1; s >= buf; --s) {
            undo(s);
        }
        --level;
    }
    
    SymbolData get(u64 key) {
        HashEntry *e = map.get(key);
        return e ? e->d : SymbolData(KIND_EMPTY, 0, 0);
    }

    void set(u64 key, int kind, int slot) {
        set(undoLog + level, key, SymbolData(kind, level, slot));
    }

    void set(int level, u64 key, int slot) {
        set(undoLog + level, key, SymbolData(kind, level, slot));
    }    

    SymbolData get(const char *str) {
        return get(hash64(str));
    }

    void set(const char *str, int kind, int slot) {
        set(hash64(str), kind, slot);
    }
};
