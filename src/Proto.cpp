#include "Proto.h"

Proto::Proto(Proto *up) :
    nArgs(0),
    level(0),
    localsTop(0),
    up(up) {
    if (up) {
        level = up->level + 1;
    }
    /*
    ups.push(-1); // NIL
    ups.push(-2); // ""
    ups.push(-3); // []
    ups.push(-4); // {}
    */
}

Proto::~Proto() {
}
