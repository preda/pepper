#include "Func.h"
#include "Proto.h"
#include "Value.h"
#include "GC.h"
#include "VM.h"

#include <stdlib.h>
#include <assert.h>

Func::Func(Proto *proto, Value *contextUps, Value *regs, byte recSlot) :
    type(O_FUNC)
{
    this->proto = proto;
    if (recSlot != 0xff) {
        regs[recSlot] = VAL_OBJ(this);
    }
    int n = nUp();
    ups = n ? (Value *) calloc(n, sizeof(Value)) : 0;
    Value *up = ups + n - 1;
    for (int *p = proto->ups.buf(), *end = p+n; p < end; ++p, --up) {
        int slot = *p;
        *up = slot >= 0 ? regs[slot] : contextUps[slot];
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

void Func::traverse(GC *gc) { 
    gc->markValVect(ups, nUp());
    gc->mark((Object *) proto);
}

void Func::setUp(int slot, Value v) {
    assert(slot > N_CONST_UPS);
    slot -= N_CONST_UPS;
    int n = nUp();
    assert(slot <= n);
    ups[n - slot] = v;
}
