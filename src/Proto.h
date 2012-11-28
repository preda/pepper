#pragma once

#include "Vector.h"
#include "Value.h"

class Proto {
    byte type;
    Proto(Proto *up);

 public:
    signed char nArgs; // negative if hasEllipsis
    byte level;
    byte localsTop;
    int patchPos;
    Vector<short> ups;
    Vector<Object *> consts;
    Vector<unsigned> code;
    Proto *up; // level-1

    ~Proto();
    static Proto *alloc(Proto *up);
    void traverse();
    void freeze() {}
};
