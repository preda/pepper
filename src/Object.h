#pragma once

#define FNV 16777619
#define PTR_HASH(p) ((((unsigned)(long)(p))>>4) * FNV)

typedef unsigned char byte;

class GC;

class Object {
    unsigned _size;

 public:
    unsigned hashCode();
    unsigned type() { return _size & 0xf; }
    unsigned size() { return _size >> 4; }
    void setType(unsigned t);
};
