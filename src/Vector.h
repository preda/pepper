#pragma once

template <typename T>
class Vector {
 public:
    unsigned size;
    T *buf;

    Vector(unsigned sizeHint = 0);
    ~Vector();

    void incSize() {
        if (size & (size-1)) {
            ++size;
        } else {
            setSize(size + 1);
        }
    }

    void setSize(unsigned newSize);
    void clear() { setSize(0); }

    void append(Vector<T> *v);

    unsigned push(T v) {
        incSize();
        buf[size - 1] = v;
        return size - 1;
    }
    
    T *push() {
        incSize();
        return buf + (size - 1);
    }

    T *top() { return buf + (size-1); }

    T pop() { return buf[--size]; }

    void set(unsigned pos, T v) {
        if (pos < size) {
            buf[pos] = v;
        } else {
            setSize(pos+1);
            buf[pos] = v;
        }
    }

    void removeRange(unsigned a, unsigned b);
    void remove(unsigned pos) { removeRange(pos, pos + 1); }
};
