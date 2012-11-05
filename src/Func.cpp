#include "Func.h"
#include "GC.h"
#include <new>
#include <stdlib.h>

Func *Func::alloc(Proto *proto, Func *parent, Value *regs) {
    return new (GC::alloc(FUNC, sizeof(Func), true)) Func(proto, parent, regs);
}

Func::Func(Proto *proto, Func *context, Value *regs) {
    this->proto = proto;
    int nup = proto->ups.size;
    ups = nup ? (Value *) calloc(nup, sizeof(Value)) : 0;
    Value *up = ups;
    Value *contextUps = context->ups;
    Value *consts = proto->consts.buf;
    for (short *p = proto->ups.buf, *end = p+nup; p < end; ++p, ++up) {
        int where = *p;
        *up = where > 0 ? regs[where-1] : where < 0 ? contextUps[-where-1] : *consts++;
    }
}

void Func::destroy() {
    if (ups) {
        free(ups);
        ups = 0;
    }
    proto = 0;
    type  = 0;
}

void Func::traverse() { 
    GC::markVector(ups, proto->ups.size);
}
