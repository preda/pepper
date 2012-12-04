#pragma once

#include "Value.h"

class Func;

struct RetInfo {
    unsigned *pc;
    Func  *func;
    unsigned base;
};
