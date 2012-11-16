#pragma once

#include "Vector.h"
#include "Value.h"

class Proto {
 public:
    Proto();

    byte nArgs;
    bool hasEllipsis;
    byte level;
    byte localsTop;

    Vector<short> ups;
    Vector<Value> consts;
    Vector<unsigned> code;
    Proto *up; // level-1
};
