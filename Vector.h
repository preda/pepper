#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

template <typename T>
struct Vector {
    int size, allocSize;
    T *buf;

    Vector(int iniSize = 0) {
        printf("Vector %p\n", this);
        allocSize = 0;
        size = 0;
        buf = 0;
        reserve(iniSize);
    }

    ~Vector() {
        if (buf) {
            free(buf);
            buf = 0;
        }
    }

    void reserve(int capacity) {
        if (capacity > allocSize) {
            while (capacity > allocSize) {
                allocSize += allocSize ? 4 : allocSize;
            }
            buf = (T*) realloc(buf, allocSize * sizeof(T));
            // memset(buf+oldAlloc, 0, (allocSize - oldAlloc) * sizeof(T));
        }
    }

    void append(Vector<T> *v) {
        reserve(size + v->size);
        memcpy(buf + size, v->buf, v->size * sizeof(T));
    }
    
    T *push() {
        reserve(size + 1);
        return buf + size++;
    }

    T *top() {
        return buf + (size-1);
    }

    void push(T v) {
        add(size, v);
    }

    T pop() {
        return buf[--size];
    }

    void set(int pos, T v) {
        if (pos < 0) { 
            return; // TODO
        }
        if (pos < size) {
            buf[pos] = v;
        } else {
            reserve(pos+1);
            buf[pos] = v;
            size = pos + 1;
        }
    }

    void add(int pos, T v) {        
        reserve(size+1);
        if (pos < size) {
            memmove(buf + pos + 1, buf + pos, (size - pos) * sizeof(T));            
        }
        buf[size++] = v;
    }

    void remove(int pos) {
        removeRange(pos, pos + 1);
    }

    void removeRange(int a, int b) {
        if (b < size) {
            memmove(buf + a, buf + b, (size - b) * sizeof(T));
        }
        size -= b - a;
    }
};
