#pragma once

#include "Vector.h"
#include "Value.h"

class Array {
    Array(int iniSize);

    Value _get(s64 pos);
    void  _set(s64 pos, Value val);

 public:
    byte type;
    Vector<Value> vect;

    ~Array();
    static Array *alloc(int iniSize = 0);
    static Array *alloc(Vector<Value> *vect);
    static Value get(Value arr, Value pos);
    static void  set(Value arr, Value pos, Value v);
    
    void traverse();

    void push(Value val) { vect.push(val); }
    unsigned size() { return vect.size; }

    bool appendArray(Value v);
    void appendArray(char *s, int size);
    void appendArray(Array *a) { vect.append(&(a->vect)); }
};
