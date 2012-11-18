#include "Vector.h"
#include "Value.h"
#include "Object.h"
#include "SymbolMap.h"
#include "RetInfo.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

template<typename T>
Vector<T>::Vector(unsigned sizeHint) : size(0), buf((T *) calloc(2, sizeof(T))) {
    if (sizeHint) {
        setSize(sizeHint);
        setSize(0);
    }
}

template<typename T>
Vector<T>::~Vector() {
    if (buf) { free(buf); }
    buf  = 0;
    size = 0;
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
    if (!sameAllocSize(size, newSize)) {
        buf = (T *) realloc(buf, getAllocSize(newSize) * sizeof(T));
    }
    size = newSize;
}

template<typename T>
void Vector<T>::append(Vector<T> *v) {
    unsigned oldSize = size;
    setSize(oldSize + v->size);
    memcpy(buf + oldSize, v->buf, v->size * sizeof(T));
}

template<typename T>
void Vector<T>::removeRange(unsigned a, unsigned b) {
    assert(b <= size);
    if (b < size) { memmove(buf + a, buf + b, (size - b) * sizeof(T)); }
    setSize(size - (b - a));
}

template class Vector<unsigned>;
template class Vector<short>;
template class Vector<Value>;
template class Vector<Object*>;
template class Vector<char>;
template class Vector<RetInfo>;
template class Vector<Symbol>;


    /*
    void add(int pos, T v) {        
        reserve(size+1);
        if (pos < size) {
            memmove(buf + pos + 1, buf + pos, (size - pos) * sizeof(T));            
        }
        buf[size++] = v;
    }
    */
