// Copyright (C) 2012 - 2013 Mihai Preda

#pragma once

#include "SimpleMap.h"

class GC;
struct NameValue;

class Map {
    SimpleMap map;
    bool hasGet, hasSet;

    Map();
    void grow();
    void set(Vector<Value> *keys, Vector<Value> *vals);
    void set(Value *keys, Value *vals, int size);
    bool set(Value key, Value v, bool overwrite);

 public:
    static Map *alloc(GC *gc, unsigned sizeHint = 0);
    static Value value(GC *gc, unsigned n, NameValue *entries);

    ~Map();

    int size() { return map.size(); }
    void traverse(GC *gc);

    Value rawGet(Value key) { return map.get(key); }
    Value rawSet(Value key, Value val);

    Value get(Value key, bool *again);
    Value set(Value key, Value val, bool *again);
        
    Map *copy(GC *gc);
    void add(Value v);
    bool equals(Map *o);

    Value *keyBuf() { return map.keyBuf(); }
    Value *valBuf() { return map.valBuf(); }
};
