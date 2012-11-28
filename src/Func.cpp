#include "Func.h"
#include "Proto.h"
#include "Value.h"
#include "Array.h"
#include "Map.h"
#include "CFunc.h"
#include "FFI.h"

#include <stdlib.h>

Func::Func(Proto *proto, Value *contextUps, Value *regs) {
    static Value defaultUps[] = {
        NIL, ZERO, VAL_INT(1), VAL_INT(-1),
        EMPTY_STRING, VAL_OBJ(Array::alloc()), VAL_OBJ(Map::alloc()),
        VAL_OBJ(CFunc::alloc(ffiConstruct, 0)),
    };

    if (!contextUps) {
        contextUps = defaultUps;
    }
    this->proto = proto;
    int nup = proto->ups.size;
    ups = nup ? (Value *) calloc(nup, sizeof(Value)) : 0;
    Value *up = ups + nup - 1;
    for (short *p = proto->ups.buf, *end = p+nup; p < end; ++p, --up) {
        int where = *p;
        *up = where >= 0 ? regs[where] : contextUps[-where-1];
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
