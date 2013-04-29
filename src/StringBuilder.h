// Copyright (C) 2013 Mihai Preda

#include "Value.h"

class StringBuilder {
public:
    char *buf;
    int size, allocSize;

    StringBuilder();
    ~StringBuilder();

    void reserve(int space);
    void clear() { size = 0; }
    char *cstr();

    void append(const char *s);
    void append(const char *s, int len);
    void append(char c);
    void append(double d);
    void append(Value v);
};
