// Copyright (C) 2012 Mihai Preda

#include "Array.h"
#include "String.h"
#include "Value.h"
#include "Object.h"
#include "GC.h"
#include "CFunc.h"
#include <assert.h>

Array::Array(int iniSize) : vect(iniSize) {
    ((Object *) this)->setType(O_ARRAY);
}

Array::~Array() {
}

void Array::traverse(GC *gc) {
    gc->markValVect(vect.buf(), vect.size());
}

Array *Array::alloc(GC *gc, Vector<Value> *v) {
    Array *a = alloc(gc, v->size());
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

Value Array::getI(int pos) { 
    int sz = size();
    if (pos < 0) { pos += sz; }
    if (pos >= sz || pos < 0) { return VNIL; } // index out of range
    return vect.get(pos); 
}

void Array::setI(int pos, Value v) {
    int sz = size();
    if (pos < 0) { pos += sz; }
    ERR(pos < 0, E_SET_NEGATIVE_INDEX);
    vect.setExtend(pos, v);
}

Value Array::indexGet(Value pos) {
    return IS_NUM(pos) ? getI(GET_NUM(pos)) : VERR;
}

bool Array::indexSet(Value pos, Value v) {
    if (IS_NUM(pos)) {
        setI(GET_NUM(pos), v);
        return true;
    } else {
        return false;
    }
}

Value Array::getSliceI(GC *gc, int pos1, int pos2) {
    int sz = size();
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
    Array *a = Array::alloc(gc);
    a->append(vect.buf() + pos1, (pos1 < pos2) ? (unsigned)(pos2-pos1) : 0);
    return VAL_OBJ(a);
}

Value Array::getSliceV(GC *gc, Value p1, Value p2) {
    ERR(!IS_NUM(p1) || !IS_NUM(p2), E_INDEX_NOT_NUMBER);
    return getSliceI(gc, GET_NUM(p1), GET_NUM(p2));
}

void Array::setSliceV(Value pos1, Value pos2, Value v) {
    // TODO
}

void Array::append(char *s, int size) {
    Value c = String::value(0, 1);
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

void Array::setSize(unsigned sz) {
    vect.setSize(sz);
}

Value Array::sizeField(VM *vm, int op, void *data, Value *stack, int nArgs) {
    if (op != CFunc::CFUNC_CALL) {
        return VNIL;
    }
    assert(nArgs > 0);
    Value v = stack[0];
    if (!IS_ARRAY(v)) {
        return VNIL;
    }
    Array *a = ARRAY(v);
    unsigned sz = 0;
    if (nArgs == 1) {
        sz = a->size();
    } else if (nArgs >= 2) {
        if (!IS_NUM(stack[1])) { return VNIL; }
        sz = (unsigned) GET_NUM(stack[1]);
        a->setSize(sz);
    }
    return VAL_NUM(sz);
}
