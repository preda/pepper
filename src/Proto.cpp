#include "Proto.h"
#include "GC.h"
#include <new>

Proto::Proto(Proto *up) :
    nArgs(0),
    level(0),
    localsTop(0),
    up(up) {
    if (up) {
        level = up->level + 1;
    }
}

Proto::~Proto() {
}

Proto *Proto::alloc(Proto *up) {
    return new (GC::alloc(PROTO, sizeof(Proto), true)) Proto(up);
}

void Proto::traverse() {
    GC::mark((Object*) up);
    GC::markVector(consts.buf, consts.size);
}
