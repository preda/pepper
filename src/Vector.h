#pragma once

template <typename T>
class Vector {
    unsigned _size;
    T *_buf;    

 public:
    T *buf() { return _buf; } // { return (T*)((long)_buf & ~7l); }
    int size() { return _size >> 4; }

    Vector(unsigned sizeHint = 0);
    ~Vector();

    void incSize() {
        const unsigned s = size();
        if (s & (s - 1)) {
            _size += (1<<4);
        } else {
            setSize(s + 1);
        }
    }

    void decSize() {
        const unsigned s = size() - 1;
        if (s & (s - 1)) {
            _size -= (1<<4);
        } else {
            setSize(s);
        }
    }

    void setSize(unsigned newSize);
    void clear() { setSize(0); }

    void append(const T *v, unsigned size);
    void append(Vector<T> *v) { append(v->buf(), v->size()); }


    unsigned push(T v) {
        incSize();
        buf()[size() - 1] = v;
        return size() - 1;
    }

    T *top() { return buf() + (size()-1); }
    
    T *push() {
        incSize();
        return top();
    }

    T pop() {
        // assert(size() > 0);
        T ret = *top();
        decSize();
        return ret;
    }

    T get(int pos);
    T operator[](int pos) { return buf()[pos]; }
    
    void setDirect(int pos, T v);
    void setExtend(int pos, T v);

    // If a < b, removes the range [a, b); If a > b, inserts the range [b, a).
    void removeRange(int a, int b);
    
    void remove(unsigned pos) { removeRange(pos, pos + 1); }
    void insertAt(int pos, T v);
};
