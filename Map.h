#pragma once

#include "Array.h"

struct Map {
    static const int EMPTY = -1;
    // static const int HOLE  = -2;

    byte type;
    int size, n;
    Value *buf;

    void grow();

    static Map *alloc(int iniSize);
        
    Map(int iniSize);
    void destroy();

    Map *copy();
    void traverse() {
        GC::markVector(buf, size);
        GC::markVector(buf+(n>>1), size);
    }

    Value get(Value key);
    bool set(Value key, Value v, bool overwrite);
    bool remove(Value key);

    void appendString(char *s, int len);
    void appendMap(Map *m);
    void appendArray(Array *a);

    bool appendArray(Value v) {
        int t = TAG(v);
        if (t == ARRAY || t == MAP || t == STRING) {
            return true;
        }
        if (t >= STRING+1 && t <= STRING+6) {
            appendString((char *) &v, t - STRING);
            return true;
        }
        if (t == OBJECT && v) {
            t = ((Object *) v)->type;
            if (t == MAP) {
                appendMap((Map *) v);
                return true;
            }
            if (t == ARRAY) {
                appendArray((Array *) v);
                return true;
            }
            if (t == STRING) {
                String *s = (String *) v;
                appendString(s->s, s->size);
                return true;
            }
        }
        return false;
    }
};
