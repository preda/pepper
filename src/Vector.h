#pragma once

template <typename T>
class Vector {
 public:
    int size, allocSize;
    T *buf;

    Vector(int iniSize = 0);
    ~Vector();

    void append(Vector<T> *v);

    void push(T v) {
        reserve(size+1);
        buf[size++] = v;
    }
    
    T *push() {
        reserve(size + 1);
        return buf + size++;
    }

    T *top() {
        return buf + (size-1);
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

    void removeRange(int a, int b);
    void remove(int pos) { removeRange(pos, pos + 1); }

    void reserve(int capacity) { if (capacity > allocSize) { doReserve(capacity); } }

 private:
    void doReserve(int capacity);
};
