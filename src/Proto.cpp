#include "Proto.h"
#include "Object.h"
#include "GC.h"
#include "VM.h"

void Proto::traverse(GC *gc) {
    gc->mark((Object*) up);
    gc->markObjVect(consts.buf(), consts.size());
}

Proto::Proto(Proto *up) :
    type(O_PROTO),
    nArgs(0),
    maxLocalsTop(0),
    patchPos(-1),
    up(up) {
}

Proto::~Proto() {
}

int Proto::newUp(int slot) {
    ups.push(slot);
    return -nUp() - N_CONST_UPS;
}
