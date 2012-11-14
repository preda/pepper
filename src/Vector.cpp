#include "Vector.h"
#include "Value.h"
#include "Object.h"
#include "SymbolMap.h"
#include "RetInfo.h"

#include <stdlib.h>
#include <string.h>

template<typename T>
Vector<T>::Vector(int iniSize) {
    allocSize = 0;
    size = 0;
    buf = 0;
    reserve(iniSize);
}

template<typename T>
Vector<T>::~Vector() {
    if (buf) {
        free(buf);
        buf = 0;
    }
}

template<typename T>
void Vector<T>::doReserve(unsigned capacity) {
    while (capacity > allocSize) {
        allocSize += allocSize ? allocSize : 4;
    }
    buf = (T*) realloc(buf, allocSize * sizeof(T));
    // memset(buf+oldAlloc, 0, (allocSize - oldAlloc) * sizeof(T));
}

template<typename T>
void Vector<T>::append(Vector<T> *v) {
    reserve(size + v->size);
    memcpy(buf + size, v->buf, v->size * sizeof(T));
}

template<typename T>
void Vector<T>::removeRange(unsigned a, unsigned b) {
    if (b < size) {
        memmove(buf + a, buf + b, (size - b) * sizeof(T));
    }
    size -= b - a;
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
