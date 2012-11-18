#pragma once

#include "common.h"
#include "Value.h"

class String {
 public:
    static String *alloc(const char *p, int size);
    static String *alloc(int size);
    static unsigned hashCode(char *buf, int size);
    static Value concat(Value a, Value b);

    byte type;
    unsigned size;
    unsigned cachedHash;
    char s[4];

    ~String();
    void traverse() {}
    unsigned hashCode();
    bool equals(String *other);
};
