#include "Array.h"
#include "String.h"
#include "Value.h"
#include "Object.h"
#include <assert.h>

Array::Array(int iniSize) : vect(iniSize) {
    ((Object *) this)->setType(O_ARRAY);
}

Array::~Array() {
}

Array *Array::alloc(Vector<Value> *v) {
    Array *a = alloc(v->size());
    a->vect.append(v);
    return a;
}

bool Array::lessThan(Array *a) {
    if (size() != a->size()) {
        return size() < a->size();
    }
    for (Value *p1=buf(), *end=p1+size(), *p2=a->buf(); p1<end; ++p1, ++p2) {
        if (::lessThan(*p1, *p2)) {
            return true;
        } else if (::lessThan(*p2, *p1)) {
            return false;
        }
    }
    return false;
}

bool Array::equals(Array *a) {
    if (size() != a->size()) { return false; }
    for (Value *p1=buf(), *end=p1+size(), *p2=a->buf(); p1<end; ++p1, ++p2) {
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
    return vect.get(pos); 
}

void Array::_set(s64 pos, Value v) {
    unsigned sz = size();
    if (pos < 0) { pos += sz; }
    ERR(pos < 0, E_SET_NEGATIVE_INDEX);
    vect.set((unsigned) pos, v);
}

Value Array::get(Value pos) {
    ERR(!IS_NUM(pos), E_INDEX_NOT_NUMBER);
    return _get((s64) GET_NUM(pos));
}

Value Array::getSlice(Value p1, Value p2) {
    ERR(!IS_NUM(p1) || !IS_NUM(p2), E_INDEX_NOT_NUMBER);
    s64 pos1 = (s64) GET_NUM(p1);
    s64 pos2 = (s64) GET_NUM(p2);
    unsigned sz = size();
    if (pos1 < 0) { 
        pos1 += sz;
        if (pos1 < 0) {
            pos1 = 0;
        }
    }
    if (pos2 < 0) { 
        pos2 += sz; 
    } else if (pos2 > sz) { 
        pos2 = sz;
    }    
    Array *a = Array::alloc();
    a->append(vect.buf() + pos1, (pos1 < pos2) ? (unsigned)(pos2-pos1) : 0);
    return VAL_OBJ(a);
}

void Array::setSlice(Value pos1, Value pos2, Value v) {
    // TODO
}

void Array::set(Value pos, Value v) {
    ERR(!IS_NUM(pos), E_INDEX_NOT_NUMBER);
    _set((s64) GET_NUM(pos), v); 
}

void Array::append(char *s, int size) {
    Value c = String::makeVal(1);
    char *pc = GET_CSTR(c);
    unsigned oldSize = vect.size();
    vect.setSize(oldSize + size);
    for (Value *p = buf()+oldSize, *end = p + size; p < end; ++p, ++s) {
        *pc = *s;
        *p  = c;            
    }
}

void Array::add(Value v) {
    ERR(!(IS_ARRAY(v) || IS_STRING(v) || IS_MAP(v)), E_ADD_NOT_COLLECTION);
    if (IS_STRING(v)) {
        append(GET_CSTR(v), len(v));
    } else {
        append((Array *) GET_OBJ(v)); // handles both MAP & ARRAY
    }
}
