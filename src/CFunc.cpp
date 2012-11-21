#include "CFunc.h"

CFunc::CFunc(tfunc f) : func(f) {
}

CFunc::~CFunc() {
    func(2, data, 0, 0);
}
