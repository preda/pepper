#pragma once

#include "common.h"
#include "Value.h"

class Map;

class String {
 private:
    unsigned _size;    

 public:
    static void staticInit();

    static Value makeVal(const char *s);
    static Value makeVal(const char *s, unsigned size);
    static Value makeVal(unsigned size);

    static String *alloc(const char *p, unsigned size);
    static String *alloc(unsigned size);
    static unsigned hashCode(char *buf, int size);
    static Value concat(Value a, Value b);
    static Value get(Value a, Value pos);

    char s[0];

    ~String();
    void traverse() {}
    unsigned hashCode();
    bool equals(String *other);
    unsigned size() { return _size >> 4; }
    void setSize(unsigned s) { _size = (s << 4) | O_STR; }

    // static int find(Valua a, Value b);
    static Value method_find(int op, void *data, Value *stack, int nCallArg);

    static Map *methods;
};
