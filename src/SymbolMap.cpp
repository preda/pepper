#include "SymbolMap.h"
#include <stdlib.h>

#define INC(p) p = (p + 1) & mask

SymbolMap::SymbolMap() {
    size = 0;
    allocSize = 32;
    tab = (HashEntry *) calloc(allocSize, sizeof(HashEntry));
}

SymbolMap::~SymbolMap() {
    free(tab);
    tab = 0;
}

HashEntry *SymbolMap::get(u64 key) {
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

HashEntry *SymbolMap::add(u64 key) {
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
