#pragma once

#include "Vector.h"
#include "Value.h"
#include "GC.h"
#include "Object.h"

struct Array {
    byte type;
    Vector<Value> vect;

    static Array *alloc(int iniSize);
    
    static int len(Value v) {
        return TAG(v)==ARRAY ? 0 : ((Array *) v)->size();
    }


    Array(int iniSize) : 
    vect(iniSize) {
    }

    void destroy() {
        vect.~Vector();
    }

    void traverse() { GC::markVector(vect.buf, vect.size); }

    Value get(int pos) {
        return pos < 0 || pos >= vect.size ? NIL : vect.buf[pos];
    }

    void set(int pos, Value val) {
        vect.set(pos, val);
    }

    void push(Value val) {
        vect.push(val);
    }

    int size() {
        return vect.size;
    }

    bool appendArray(Value v) {
        int t = TAG(v);
        switch (t) {
        case ARRAY: case MAP: case STRING: return true;
        case STRING+1: case STRING+2: case STRING+3: case STRING+4: case STRING+5: case STRING+6:
            appendArray((char *) &v, t - STRING);
            return true;

        case OBJECT: break;

        default: return false;
        }
        
        if (!v) { return false; }
        
        switch (((Object *) v)->type) {
        case ARRAY:
            appendArray((Array *) v);
            return true;
            
        case STRING: {
            String *s = (String *) v;
            appendArray(s->s, s->size);
            return true;
        }

        case MAP:
            appendArray((Array *) v);
            return true;

        default:
            return false;
        }
    }

    void appendArray(Array *a) {
        vect.append(&(a->vect));
    }

    void appendArray(char *s, int size) {
        Value c = VALUE(STRING+1, 0);
        vect.reserve(vect.size + size);
        Value *p = vect.buf + vect.size;
        vect.size += size;
        for (char *end = s + size; s < end; ++s, ++p) {
            *((char *) &c) = *s;
            *p = c;            
        }
    }

    // void remove(int pos) { v.remove(pos); }
};
