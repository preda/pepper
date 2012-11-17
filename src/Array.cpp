#include "Array.h"
#include "String.h"
#include "GC.h"
#include "Value.h"
#include "Object.h"
#include <new>

Array::Array(int iniSize) : vect(iniSize) { }

Array *Array::alloc(int iniSize) {
    return new (GC::alloc(ARRAY, sizeof(Array), true)) Array(iniSize);
}

Array *Array::alloc(Vector<Value> *v) {
    Array *a = alloc(v->size);
    a->vect.append(v);
    return a;
}

bool Array::appendArray(Value v) {
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
