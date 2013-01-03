#include "Vector.h"
#include "Value.h"
#include "Object.h"
#include "RetInfo.h"
#include "SymbolTable.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

template<typename T>
Vector<T>::Vector(unsigned sizeHint) :
  _size(0), 
  _buf((T *) calloc(2, sizeof(T)))
{
    if (sizeHint) {
        setSize(sizeHint);
        setSize(0);
    }
}

template<typename T>
Vector<T>::~Vector() {
    free(buf());
    _buf  = 0;
    _size = 0;
}

static bool sameAllocSize(unsigned size1, unsigned size2) {
    if (size1 <= 2 || size2 <= 2) {
        return size1 <= 2 && size2 <= 2;
    }
    --size1;
    --size2;
    const unsigned x = size1 ^ size2;
    return x < size1 && x < size2;
}

static size_t getAllocSize(unsigned size) {
    size_t a = 2;
    while (a < size) { a += a; }
    return a;
}

template<typename T>
void Vector<T>::setSize(unsigned newSize) {
    if (!sameAllocSize(size(), newSize)) {
        _buf = (T *) realloc(buf(), getAllocSize(newSize) * sizeof(T));
    }
    _size = (newSize << 4) | (_size & 0xf);
}

template<typename T>
void Vector<T>::append(T *v, unsigned vSize) {
    unsigned oldSize = size();
    setSize(oldSize + vSize);
    memcpy(buf() + oldSize, v, vSize * sizeof(T));
}

template<typename T>
void Vector<T>::removeRange(unsigned a, unsigned b) {
    assert(b <= size());
    if (b < size()) { memmove(buf() + a, buf() + b, (size() - b) * sizeof(T)); }
    setSize(size() - (b - a));
}

template class Vector<unsigned>;
template class Vector<short>;
template class Vector<Value>;
template class Vector<Object*>;
template class Vector<char>;
template class Vector<RetInfo>;
template class Vector<SymbolTable::UndoEntry>;
