#pragma once

#include "Value.h"

class Array;
class String;

struct Map {
    static const int EMPTY = -1;
    // static const int HOLE  = -2;

    byte type;
    unsigned size, n;
    Value *buf;

    void grow();

    static Map *alloc(unsigned iniSize);
        
    Map(unsigned iniSize);
    void destroy();

    Map *copy();
    void traverse();

    Value get(Value key);
    bool set(Value key, Value v, bool overwrite);
    bool remove(Value key);

    void appendString(char *s, int len);
    void appendMap(Map *m);
    void appendArray(Array *a);
    bool appendArray(Value v);
};
