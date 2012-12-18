#pragma once

#include "Vector.h"
#include "SymbolMap.h"
#include "common.h"

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

    SymbolData set(SymbolVect *currUndo, u64 key, SymbolData data) {
        Symbol *s = currUndo->push();
        s->key    = key;
        HashEntry *e = map.add(key);
        s->d = e->d;
        e->d = data;
        return data;
    }
    
 public:
    SymbolTable();
    ~SymbolTable();

    void print();

    int pushContext();
    int popContext();
    
    SymbolData get(u64 key) {
        HashEntry *e = map.get(key);
        return e ? e->d : SymbolData(KIND_EMPTY, 0, 0);
    }

    SymbolData set(u64 key, int slot) {
        return set(undoLog + level, key, SymbolData(KIND_REGUP, level, slot));
    }

    SymbolData set(int level, u64 key, int slot) {
        return set(undoLog + level, key, SymbolData(KIND_REGUP, level, slot));
    }    

    SymbolData get(const char *str) {
        return get(hash64(str));
    }

    SymbolData set(const char *key, int slot) {
        return set(hash64(key), slot);
    }
};
