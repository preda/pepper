#include "Proto.h"
#include "VM.h"

Proto::Proto() :
    nArgs(0),
    hasEllipsis(false),
    level(0),
    top(0),
    up(0) {
}

void Proto::print() {
    bytecodePrint(code.buf, code.size);
}
