#pragma once

#include "Value.h"

struct RetInfo {
    unsigned *pc;
    Value *regs;
    Value *ups;
};
