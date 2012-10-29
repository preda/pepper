#include "GC.h"
#include <new>

Func *Func::alloc(Proto *proto, Func *parent, Value *regs) {
    return new (GC::alloc(FUNC, sizeof(Func), true)) Func(proto, parent, regs);
}
