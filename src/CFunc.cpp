#include "CFunc.h"
#include "GC.h"

CFunc *CFunc::alloc(void *f, int dataSize) {
    return GC::alloc(CFUNC, sizeof(CFunc) + dataSize, true);
}
