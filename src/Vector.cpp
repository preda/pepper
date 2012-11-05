#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Vector.h"
#include "Value.h"

template<typename T>
Vector<T>::Vector(int iniSize) {
    printf("Vector %p\n", this);
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
void Vector<T>::doReserve(int capacity) {
    while (capacity > allocSize) {
        allocSize += allocSize ? 4 : allocSize;
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
void Vector<T>::removeRange(int a, int b) {
    if (b < size) {
        memmove(buf + a, buf + b, (size - b) * sizeof(T));
    }
    size -= b - a;
}

template class Vector<unsigned>;
template class Vector<short>;
template class Vector<Value>;

    /*
    void add(int pos, T v) {        
        reserve(size+1);
        if (pos < size) {
            memmove(buf + pos + 1, buf + pos, (size - pos) * sizeof(T));            
        }
        buf[size++] = v;
    }
    */
