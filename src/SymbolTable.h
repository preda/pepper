#pragma once

#include "Vector.h"
#include "common.h"
#include "SymbolMap.h"

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

    SymbolData set(u64 key, int kind, int slot) {
        return set(undoLog + level, key, SymbolData(kind, level, slot));
    }

    SymbolData set(int level, u64 key, int slot) {
        return set(undoLog + level, key, SymbolData(KIND_REGUP, level, slot));
    }    

    SymbolData get(const char *str) {
        return get(hash64(str));
    }

    /*
    void set(const char *str, int kind, int slot) {
        set(hash64(str), kind, slot);
    }
    */
};
