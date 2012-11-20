#include "Func.h"
#include "Proto.h"
#include "Value.h"
#include "Array.h"
#include "Map.h"

#include <stdlib.h>

Func::Func(Proto *proto, Value *contextUps, Value *regs) {
    static Value defaultUps[] = {NIL, EMPTY_STRING, VAL_OBJ(Array::alloc()), VAL_OBJ(Map::alloc())};
    if (!contextUps) {
        contextUps = defaultUps;
    }
    this->proto = proto;
    int nup = proto->ups.size;
    ups = nup ? (Value *) calloc(nup, sizeof(Value)) : 0;
    Value *up = ups;
    Value *consts = proto->consts.buf;
    for (short *p = proto->ups.buf, *end = p+nup; p < end; ++p, ++up) {
        int where = *p;
        *up = where > 0 ? regs[where-1] : where < 0 ? contextUps[-where-1] : *consts++;
    }
}

Func::~Func() {
    if (ups) {
        free(ups);
        ups = 0;
    }
    proto = 0;
    type  = 0;
}
