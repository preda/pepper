#pragma once

#include "Vector.h"
#include "Value.h"

#define N_CONST_UPS (7)

class GC;

class Proto {
    byte type;
    Proto(Proto *up);

 public:
    Vector<int> ups;
    signed char nArgs; // negative if hasEllipsis
    // byte level;
    byte localsTop;
    int patchPos;

    Vector<Object *> consts;
    Vector<int> code;
    Proto *up; // level-1

    unsigned nUp() { return ups.size(); }
    void addUp(int slot) { ups.push(slot); }

    ~Proto();
    static Proto *alloc(GC *gc, Proto *up);
    void traverse(GC *gc);
    void freeze() {}
};
