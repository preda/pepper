#pragma once

#include "Vector.h"
#include "Object.h"
#include "value.h"

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
    int newUp(int slot);

    ~Proto();
    static Proto *alloc(GC *gc, Proto *up);
    void traverse(GC *gc);
    void freeze() {}
};
