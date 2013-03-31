#include "Stack.h"
#include <stdlib.h>
#include <assert.h>

Stack::Stack() :
    base((Value *) calloc(512, sizeof(Value))),
    size(512),
    top(0)
{
}

Stack::~Stack() {
    if (base) {
        free(base);
    }
    base = 0;
    size = 0;
    top  = 0;
}

Value *Stack::maybeGrow(Value *regs, unsigned space) {
    if (!regs) { regs = base + top; }
    assert(base <= regs && regs < base + size);
    if (regs + space > base + size) {
        int pos = regs - base;
        size += max(512, space);
        base = (Value *) realloc(base, size * sizeof(Value));
        return base + pos;
    } else {
        return regs;
    }
}

void Stack::shrink() {
    size = 512;
    base = (Value *) realloc(base, size * sizeof(Value));
}
