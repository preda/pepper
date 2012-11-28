#pragma once

#include "Value.h"

class Func;

struct RetInfo {
    unsigned *pc;
    Value *regs;
    Func  *func;
};
