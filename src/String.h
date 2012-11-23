#pragma once

#include "common.h"
#include "Value.h"

class String {
 public:
    static Value makeVal(const char *s);
    static Value makeVal(const char *s, unsigned size);
    static Value makeVal(unsigned size);

    static String *alloc(const char *p, unsigned size);
    static String *alloc(unsigned size);
    static unsigned hashCode(char *buf, int size);
    static Value concat(Value a, Value b);
    static Value get(Value a, Value pos);

    byte type;
    unsigned size;
    char s[0];

    ~String();
    void traverse() {}
    unsigned hashCode();
    bool equals(String *other);
};
