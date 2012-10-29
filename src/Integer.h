#pragma once

#include "Value.h"

struct Integer {
    byte type;
    __int128 val;
    
    Integer *alloc();
    unsigned hashCode();
};
