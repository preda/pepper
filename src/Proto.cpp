#include "Proto.h"
#include "Object.h"

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

Proto::Proto(Proto *up) :
    type(O_PROTO),
    nArgs(0),
    level(0),
    localsTop(0),
    patchPos(-1),
    up(up) {
    level = (up ? up->level : 0) + 1;
}

Proto::~Proto() {
}
