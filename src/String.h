#pragma once

// #include "Object.h"

struct String {
    byte type;
    int size;
    unsigned cachedHash;
    char s[4];

    static String *alloc(const char *p, int size);
    static String *alloc(int size);

    // no traverse
    
    static unsigned hashCode(char *buf, int size);
    unsigned hashCode();
    bool equals(String *other);
};
