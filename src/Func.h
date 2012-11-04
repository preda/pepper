#pragma once

#include "Vector.h"
#include "Value.h"
#include "GC.h"

struct Proto {
    byte nArgs;
    bool hasEllipsis;
    Vector<short> ups;
    Vector<unsigned> code;
    Proto *parent;
};

struct Func {
    byte type;
    Proto *proto;
    Value *ups;

    static Func *alloc(Proto *proto, Func *parent, Value *regs);

    Func(Proto *proto, Func *context, Value *regs) {
        this->proto = proto;
        int nup = proto->nUps;
        ups = nup ? (Value *) calloc(nup, sizeof(Value)) : 0;
        Value *up = ups;
        Value *contextUps = context->ups;
        for (short *p = proto->upInfo, *end = proto->upInfo+nup; p < end; ++p, ++up) {
            int where = *p;
            *up = where >= 0 ? regs[where] : contextUps[-where-1];
        }
    }
    
    void destroy() {
        if (ups) {
            free(ups);
            ups = 0;
        }
        proto = 0;
        type  = 0;
    }

    void traverse() { 
        GC::markVector(ups, proto->nUps);
    }
};
