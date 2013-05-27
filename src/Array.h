#pragma once

#include "Vector.h"
#include "Value.h"

class GC;

class Array {
    Array(int iniSize);

    void append(Array *a) { append(a->buf(), a->size()); }
    void append(char *s, int size);

    static Array *alloc(GC *gc, Vector<Value> *vect);

 public:
    Vector<Value> vect;

    ~Array();
    static Array *alloc(GC *gc, int iniSize = 0);
    void traverse(GC *gc);

    Value getI(int pos);
    Value getV(Value pos);

    void  setI(int pos, Value val);
    void  setV(Value pos, Value v);

    Value getSliceV(GC *gc, Value pos1, Value pos2);
    Value getSliceI(GC *gc, int pos1, int pos2);

    void setSliceV(Value pos1, Value pos2, Value v);
    void setSliceI(int pos1, int pos2, Value v);
    void append(Value *buf, int size) { vect.append(buf, size); }
    
    void push(Value val) { vect.push(val); }
    unsigned size()      { return vect.size(); }
    Value *buf()         { return vect.buf(); }

    void add(Value v);
    bool equals(Array *a);
    bool lessThan(Array *a);
    Array *copy(GC *gc) { return alloc(gc, &vect); }
};
