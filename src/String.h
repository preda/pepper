#pragma once

#include "common.h"
#include "Value.h"

class Map;
class GC;
class VM;

class String {
 private:
    unsigned _size;

 public:
    static Value method_find(VM *vm, int op, void *data, Value *stack, int nCallArg);
    static Value value(GC *gc, const char *s);
    static Value value(GC *gc, const char *s, unsigned size);
    static Value value(GC *gc, unsigned size);

    static String *alloc(GC *gc, const char *p, unsigned size);
    static String *alloc(GC *gc, unsigned size);
    static unsigned hashCode(char *buf, int size);
    static Value concat(GC *gc, Value a, Value b);
    static Value get(Value a, Value pos);
    static Value getSlice(GC *gc, Value a, Value pos1, Value pos2);
    static Value __SET, __GET;

    char s[0];

    ~String();
    void traverse(GC *gc) {}
    unsigned hashCode();
    bool equals(String *other);
    unsigned size() { return _size >> 4; }
    void setSize(unsigned s) { _size = (s << 4) | O_STR; }
};
