#pragma once

#include "Value.h"

template<typename T> class Vector;
class Array;
class String;

class Map {
    unsigned _size;
    unsigned n;

    static const int EMPTY = -1;
    // static const int HOLE  = -2;
    Map(unsigned iniSize);
    void grow();
    void set(Vector<Value> *keys, Vector<Value> *vals);
    bool set(Value key, Value v, bool overwrite);

    void appendChars(char *s, int len);
    void appendMap(Map *m);
    void appendArray(Array *a);

 public:
    Value *buf;

    unsigned size() { return _size >> 4; }
    void setSize(unsigned s) { _size = (s << 4) | O_MAP; }
    void incSize() { _size += (1<<4); }

    static Map *alloc(unsigned iniSize = 0);
    static Map *alloc(Vector<Value> *keys, Vector<Value> *vals);

    Value get(Value key);
    void set(Value key, Value v) { set(key, v, true); }

    ~Map();
        
    Map *copy();
    void traverse();

    bool remove(Value key);

    void add(Value v);
    bool equals(Map *o);
};
