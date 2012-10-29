#include "Array.h"
#include "GC.h"
#include "Value.h"
#include "Object.h"
#include <new>

Array *Array::alloc(int iniSize) {
    return new (GC::alloc(ARRAY, sizeof(Array), true)) Array(iniSize);
}
