#pragma once

#include "Vector.h"
#include "Value.h"

class GC;

class Array {
    Array(int iniSize);

    Value _get(s64 pos);
    void  _set(s64 pos, Value val);

    void append(Value *buf, int size) { vect.append(buf, size); }
    void append(Array *a) { append(a->buf(), a->size()); }
    void append(char *s, int size);

    static Array *alloc(GC *gc, Vector<Value> *vect);

 public:
    Vector<Value> vect;

    ~Array();
    static Array *alloc(GC *gc, int iniSize = 0);
    void traverse(GC *gc);

    Value get(Value pos);
    void  set(Value pos, Value v);

    Value getSlice(GC *gc, Value pos1, Value pos2);
    void setSlice(Value pos1, Value pos2, Value v);
    
    void push(Value val) { vect.push(val); }
    unsigned size()      { return vect.size(); }
    Value *buf()         { return vect.buf(); }

    void add(Value v);
    bool equals(Array *a);
    bool lessThan(Array *a);
};
