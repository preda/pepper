#pragma once

#include "Vector.h"
#include "Value.h"
#include "Object.h"

class Array {
    Array(int iniSize);

 public:
    byte type;
    Vector<Value> vect;

    ~Array();
    static Array *alloc(int iniSize = 0);
    static Array *alloc(Vector<Value> *vect);
    
    void traverse();

    Value get(unsigned pos) {
        return pos < 0 || pos >= vect.size ? NIL : vect.buf[pos];
    }

    void set(unsigned pos, Value val) {
        vect.set(pos, val);
    }

    void push(Value val) {
        vect.push(val);
    }

    int size() {
        return vect.size;
    }

    bool appendArray(Value v);

    void appendArray(Array *a) {
        vect.append(&(a->vect));
    }

    void appendArray(char *s, int size) {
        Value c = VALUE(STRING+1, 0);
        vect.reserve(vect.size + size);
        Value *p = vect.buf + vect.size;
        vect.size += size;
        for (char *end = s + size; s < end; ++s, ++p) {
            *((char *) &c) = *s;
            *p = c;            
        }
    }
};
