#pragma once

#include "Vector.h"
#include "Value.h"

class Proto {
    byte type;
    Proto(Proto *up);

 public:
    signed char nArgs; // negative if hasEllipsis
    // bool hasEllipsis;
    byte level;
    byte localsTop;
    Vector<short> ups;
    Vector<Value> consts;
    Vector<unsigned> code;
    Proto *up; // level-1

    ~Proto();
    static Proto *alloc(Proto *up);
    void traverse();
    void freeze() {}
};
