#include "CFunc.h"

CFunc::CFunc(tfunc f) : 
    type(O_CFUNC),
    cfunc(f) {
}

CFunc::~CFunc() {
    cfunc(0, CFUNC_DELETE, data, 0, 0);
}

Value CFunc::value(GC *gc, tfunc f, unsigned dataSize) {
    return dataSize ? VAL_OBJ(alloc(gc, f, dataSize)) : VAL_CF(f);
}
