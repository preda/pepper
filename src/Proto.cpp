#include "Proto.h"
#include "VM.h"

Proto::Proto() :
    nArgs(0),
    hasEllipsis(false),
    level(0),
    localsTop(0),
    up(0) {
}
