#pragma once

#include "Vector.h"
#include "Value.h"
#include "GC.h"
#include "Object.h"

class Array {
    Array(int iniSize);

 public:
    byte type;
    Vector<Value> vect;

    static Array *alloc(int iniSize = 0);
    static Array *alloc(Vector<Value> *vect);
    
    /*
    static int len(Value v) {
        return TAG(v)==ARRAY ? 0 : ((Array *) v)->size();
    }
    */


    void destroy() {
        vect.~Vector();
    }

    void traverse() { GC::markVector(vect.buf, vect.size); }

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

    // void remove(int pos) { v.remove(pos); }
};
