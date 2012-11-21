#include "Array.h"
#include "String.h"
#include "Value.h"
#include "Object.h"
#include <assert.h>

Array::Array(int iniSize) : vect(iniSize) {
}

Array::~Array() {
}

Array *Array::alloc(Vector<Value> *v) {
    Array *a = alloc(v->size);
    a->vect.append(v);
    return a;
}

bool Array::equals(Array *a) {
    if (vect.size != a->vect.size) { return false; }
    for (Value *p1=vect.buf, *end=p1+vect.size, *p2=a->vect.buf; p1<end; ++p1, ++p2) {
        if (!::equals(*p1, *p2)) {
            return false;
        }
    }
    return true;
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
    ERR(!IS_INT(pos), E_INDEX_NOT_INT);
    return ((Array *) arr)->_get(getInteger(pos));
}

void Array::set(Value arr, Value pos, Value v) {
    assert(IS_ARRAY(arr));
    ERR(!IS_INT(pos), E_INDEX_NOT_INT);
    ((Array *) arr)->_set(getInteger(pos), v); 
}

void Array::appendChars(char *s, int size) {
    Value c = String::makeVal(1);
    char *pc = GET_CSTR(c);
    unsigned oldSize = vect.size;
    vect.setSize(oldSize + size);
    for (Value *p = vect.buf+oldSize, *end = p + size; p < end; ++p, ++s) {
        *pc = *s;
        *p  = c;            
    }
}

void Array::add(Value v) {
    ERR(!(IS_ARRAY(v) || IS_STRING(v) || IS_MAP(v)), E_ADD_NOT_COLLECTION);
    if (IS_STRING(v)) {
        appendChars(GET_CSTR(v), len(v));
    } else {
        appendArray((Array *) v); // handles both MAP & ARRAY
    }
}
