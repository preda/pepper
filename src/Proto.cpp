#include "Proto.h"

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
    nArgs(0),
    level(0),
    localsTop(0),
    patchPos(-1),
    up(up) {
    if (up) {
        level = up->level + 1;
    }
    ups.push(-1); // NIL
    ups.push(-2); // 0
    ups.push(-3); // 1
    ups.push(-4); // -1
    ups.push(-5); // ""
    ups.push(-6); // []
    ups.push(-7); // {}
    // ups.push(-8); // ffi
}

Proto::~Proto() {
}
