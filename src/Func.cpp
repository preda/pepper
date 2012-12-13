#include "Func.h"
#include "Proto.h"
#include "Value.h"
/*
#include "Array.h"
#include "Map.h"
#include "CFunc.h"
#include "FFI.h"
*/

#include <stdlib.h>

Func::Func(Proto *proto, Value *contextUps, Value *regs, byte recSlot) :
    type(O_FUNC)
{
    this->proto = proto;
    if (recSlot != 0xff) {
        regs[recSlot] = VAL_OBJ(this);
    }
    int n = nOwnUp();
    ups = n ? (Value *) calloc(n, sizeof(Value)) : 0;
    Value *up = ups + n - 1;
    for (short *p = proto->getUpBuf(), *end = p+n; p < end; ++p, --up) {
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
