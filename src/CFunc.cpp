#include "CFunc.h"

CFunc::CFunc(tfunc f) : 
    type(O_CFUNC),
    cfunc(f) {
}

CFunc::~CFunc() {
    cfunc(CFUNC_DELETE, data, 0, 0);
}
