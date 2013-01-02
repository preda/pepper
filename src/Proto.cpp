#include "Proto.h"
#include "Object.h"
#include "GC.h"

/*
enum {
    UP_NIL  = -1,
    UP_ZERO = -2,
    UP_ONE  = -3,
    UP_NEG_ONE = -4,
    UP_EMPTY_STRING = -5,
    UP_EMPTY_ARRAY  = -6,
    UP_EMPTY_MAP    = -7,
};
*/

void Proto::traverse(GC *gc) {
    gc->mark((Object*) up);
    gc->markObjVect(consts.buf(), consts.size());
}

Proto::Proto(Proto *up) :
    type(O_PROTO),
    nArgs(0),
    level(0),
    localsTop(0),
    patchPos(-1),
    up(up) {
    level = (up ? up->level : 0) + 1;
    for (int i = 0; i < N_CONST_UPS; ++i) {
        ups.push(-(i+1));
    }
}

Proto::~Proto() {
}
