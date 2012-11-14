#pragma once

#include "common.h"
#include "Value.h"

struct String {
    byte type;
    unsigned size;
    unsigned cachedHash;
    char s[4];

    static Value concat(Value a, Value b);

    static String *alloc(const char *p, int size);
    static String *alloc(int size);

    // no traverse

    // static int len(Value a);
    static unsigned hashCode(char *buf, int size);
    unsigned hashCode();
    bool equals(String *other);
};
