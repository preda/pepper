#pragma once

#include "Value.h"

template<typename T> class Vector;
class Array;
class String;

class Map {

    static const int EMPTY = -1;
    // static const int HOLE  = -2;
    Map(unsigned iniSize);
    void grow();
    void set(Vector<Value> *keys, Vector<Value> *vals);
    Value _get(Value key);

 public:
    byte type;
    unsigned size, n;
    Value *buf;

    static Map *alloc(unsigned iniSize);
    static Map *alloc(Vector<Value> *keys, Vector<Value> *vals);
    static Value get(Value a, Value key);

    ~Map();
        
    Map *copy();
    void traverse();

    bool set(Value key, Value v, bool overwrite);
    bool remove(Value key);

    void appendString(char *s, int len);
    void appendMap(Map *m);
    void appendArray(Array *a);
    bool appendArray(Value v);
};
