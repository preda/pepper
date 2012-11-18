#include "CFunc.h"
#include "GC.h"

CFunc *CFunc::alloc(void *f, int dataSize) {
    return (CFunc *) GC::alloc(CFUNC, sizeof(CFunc) + dataSize, true);
}

CFunc::~CFunc() {
    func(2, data, 0, 0);
}
