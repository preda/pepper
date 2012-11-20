#include "Array.h"
#include "String.h"
#include "GC.h"
#include "Value.h"
#include "Object.h"

#include <assert.h>
#include <new>

Array::Array(int iniSize) : vect(iniSize) {
}

Array::~Array() {
}

Array *Array::alloc(int iniSize) {
    return new (GC::alloc(ARRAY, sizeof(Array), true)) Array(iniSize);
}

Array *Array::alloc(Vector<Value> *v) {
    Array *a = alloc(v->size);
    a->vect.append(v);
    return a;
}

void Array::traverse() {
    GC::markVector(vect.buf, vect.size);
}

Value Array::_get(s64 pos) { 
    unsigned sz = size();
    if (pos < 0) { pos += sz; }
    if (pos >= sz || pos < 0) { return NIL; } // index out of range
    return vect.buf[(unsigned) pos]; 
}

void Array::_set(s64 pos, Value v) {
    unsigned sz = size();
    if (pos < 0) { pos += sz; }
    ERR(pos < 0, E_SET_NEGATIVE_INDEX);
    vect.set((unsigned) pos, v);
}

Value Array::get(Value arr, Value pos) {
    assert(IS_ARRAY(arr));
    ERR(!IS_INTEGER(pos), E_INDEX_NOT_INT);
    return ((Array *) arr)->_get(getInteger(pos));
}

void Array::set(Value arr, Value pos, Value v) {
    assert(IS_ARRAY(arr));
    ERR(!IS_INTEGER(pos), E_INDEX_NOT_INT);
    ((Array *) arr)->_set(getInteger(pos), v); 
}

void Array::appendChars(char *s, int size) {
    Value c = VALUE(STRING+1, 0);
    unsigned oldSize = vect.size;
    vect.setSize(oldSize + size);
    for (Value *p = vect.buf+oldSize, *end = p + size; p < end; ++p, ++s) {
        *((char *) &c) = *s;
        *p = c;            
    }
}

void Array::add(Value v) {
    assert(IS_ARRAY(v) || IS_STRING(v) || IS_MAP(v));
    if (TAG(v) >= STRING && TAG(v) <= STRING+6) {
        appendChars((char *) &v, TAG(v)-STRING);
    } else {
        assert(TAG(v) == OBJECT);
        switch (((Object *) v)->type) {
        case ARRAY:  appendArray((Array *) v); break;            
        case STRING: appendChars(((String *) v)->s, ((String *) v)->size); break;
        case MAP:    appendArray((Array *) v); break;
        }
    }
}
