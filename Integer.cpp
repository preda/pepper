#include "Integer.h"

Integer *Integer::alloc() {
    return GC::alloc(INTEGER, sizeof(Integer), false);
}

unsigned Integer::hashCode() {
    return ((unsigned) val) * FNV;
}
