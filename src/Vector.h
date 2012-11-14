#pragma once

template <typename T>
class Vector {
 public:
    unsigned size, allocSize;
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

    void set(unsigned pos, T v) {
        if (pos < size) {
            buf[pos] = v;
        } else {
            reserve(pos+1);
            buf[pos] = v;
            size = pos + 1;
        }
    }

    void removeRange(unsigned a, unsigned b);
    void remove(unsigned pos) { removeRange(pos, pos + 1); }

    void reserve(unsigned capacity) { if (capacity > allocSize) { doReserve(capacity); } }

 private:
    void doReserve(unsigned capacity);
};
