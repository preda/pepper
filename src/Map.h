// Copyright (C) 2012 - 2013 Mihai Preda

#pragma once

#include "value.h"
#include "Vector.h"
#include "Index.h"

class GC;
struct NameValue;

class Map {
    Vector<Value> vals;
    Index index;

    Map();
    void grow();
    
    void setVectors(Vector<Value> *keys, Vector<Value> *vals);
    void setArrays(Value *keys, Value *vals, int size);        
    Value remove(Value key); // returns previous value or NIL
    bool contains(Value key) { return index.getPos(key) >= 0; }
    
 public:
    static Map *alloc(GC *gc, unsigned sizeHint = 0);
    static Value makeMap(GC *gc, ...);
    static Value keysField(VM *vm, int op, void *data, Value *stack, int nArgs);
    
    ~Map();

    int size() { return index.size(); }
    void traverse(GC *gc);

    Value rawGet(Value key);
    Value rawSet(Value key, Value val);

    Value get(Value key);
    Value set(Value key, Value val);
        
    Map *copy(GC *gc);
    void add(Value v);
    bool equals(Map *o);

    Value *keyBuf() { return index.getBuf(); }
    Value *valBuf() { return vals.buf(); }
};

// bool set(Value key, Value v, bool overwrite);
// Value getOrAdd(Value key, Value v); // sets if not existing and returns get(key)
