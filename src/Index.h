// Copyright (C) 2012 - 2013 Mihai Preda

#pragma once

#include "Value.h"
#include "Vector.h"

class Index {
    Value *buf;
    int allocSize;
    int _size;

    void grow();
    void shrink();
    void remap();

 public:
    static const int EMPTY = -1;

    Index();
    ~Index();

    int size() { return _size; }
    int add(Value key);    // returns: getPos(key) before add
    int remove(Value key); // returns: position of key, -1 if not found
    int getPos(Value key); // returns: position of key, -1 if not found
    Value getVal(int pos); // 0 <= pos < size
};
