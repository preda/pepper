#pragma once

#define FNV 16777619
#define PTR_HASH(p) ((((unsigned)(long)(p))>>4) * FNV)

typedef unsigned char byte;

class GC;

struct Object {
    byte type;
    void traverse();
    void destroy();
    unsigned hashCode();
};
