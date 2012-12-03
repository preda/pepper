#pragma once

#include "Vector.h"
#include "Value.h"

#define N_CONST_UPS (7)

class Proto {
    byte type;
    Proto(Proto *up);
    Vector<short> ups;

 public:
    signed char nArgs; // negative if hasEllipsis
    byte level;
    byte localsTop;
    int patchPos;

    Vector<Object *> consts;
    Vector<unsigned> code;
    Proto *up; // level-1

    unsigned nUp() { return ups.size + N_CONST_UPS; }
    short *getUpBuf() { return ups.buf; }
    void addUp(short slot) { ups.push(slot); }

    ~Proto();
    static Proto *alloc(Proto *up);
    void traverse();
    void freeze() {}
};
