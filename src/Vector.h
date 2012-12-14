#pragma once

template <typename T>
class Vector {
    unsigned _size;
    T *_buf;    

 public:
    T *buf() { return _buf; } // { return (T*)((long)_buf & ~7l); }
    unsigned size() { return _size >> 4; }

    Vector(unsigned sizeHint = 0);
    ~Vector();

    void incSize() {
        unsigned s = size();
        if (s & (s - 1)) {
            _size += (1<<4);
        } else {
            setSize(s + 1);
        }
    }

    void setSize(unsigned newSize);
    void clear() { setSize(0); }

    void append(T *v, unsigned size);
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
        _size -= (1<<4);
        return ret;
    }

    T get(unsigned pos) {
        // assert(pos < size());
        return buf()[pos];
    }

    void set(unsigned pos, T v) {
        if (pos >= size()) {
            setSize(pos + 1);
        }
        buf()[pos] = v;
    }

    void removeRange(unsigned a, unsigned b);
    void remove(unsigned pos) { removeRange(pos, pos + 1); }
};
