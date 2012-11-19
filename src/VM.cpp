#include "VM.h"
#include "Array.h"
#include "Map.h"
#include "String.h"
#include "Func.h"
#include "CFunc.h"
#include "Proto.h"
#include "Value.h"
#include "RetInfo.h"
#include "Object.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define STEP code=*pc++; ptrC=regs+OC(code); A=regs[OA(code)]; B=regs[OB(code)]; goto *dispatch[OP(code)]

#define INT(x) VAL_INT((signed char)x)

#define DECODE(A) { byte v=O##A(code); A=v<0x80 ? VAL_INT((((signed char)(v<<1))>>1)) : v<0xf0 ? ups[v & 0x7f] : VALUE((v&0xf), 0); }

static Value arrayAdd(Value a, Value b) {
    int sz = len(a) + len(b);
    if (sz == 0) {
        int tb = TAG(b);
        return (tb == ARRAY || tb == MAP || tb == STRING) ? EMPTY_ARRAY : ERROR(E_WRONG_TYPE);
    }
    Array *array = Array::alloc(sz);
    return (array->appendArray(a) && array->appendArray(b)) ? VAL_OBJ(array) : ERROR(E_WRONG_TYPE);
}

static Value mapAdd(Value a, Value b) {
    int sz = len(a) + len(b);
    if (sz == 0) {
        int tb = TAG(b);
        return (tb == ARRAY || tb == MAP || tb == STRING) ? EMPTY_MAP : ERROR(E_WRONG_TYPE);
    }
    Map *map = ((Map *) a)->copy();
    return map->appendArray(b) ? VAL_OBJ(map) : ERROR(E_WRONG_TYPE);
}

Value doAdd(Value a, Value b) {
    return 
        IS_INTEGER(a) && IS_INTEGER(b) ? VAL_INT(getInteger(a) + getInteger(b))  :
        IS_NUMBER(a)  && IS_NUMBER(b)  ? VAL_DOUBLE(getDouble(a) + getDouble(b)) :
        IS_STRING(a) ? String::concat(a, b) :
        IS_ARRAY(a)  ? arrayAdd(a, b) :
        IS_MAP(a)    ? mapAdd(a, b) :
        ERROR(E_WRONG_TYPE);
}

Value doSub(Value a, Value b) {
    return 
        IS_INTEGER(a) && IS_INTEGER(b) ? VAL_INT(getInteger(a) - getInteger(b))  :
        IS_NUMBER(a)  && IS_NUMBER(b)  ? VAL_DOUBLE(getDouble(a) - getDouble(b)) :
        ERROR(E_WRONG_TYPE);
}

Value doMul(Value a, Value b) {
    return 
        IS_INTEGER(a) && IS_INTEGER(b) ? VAL_INT(getInteger(a) * getInteger(b))  :
        IS_NUMBER(a)  && IS_NUMBER(b)  ? VAL_DOUBLE(getDouble(a) * getDouble(b)) :
        ERROR(E_WRONG_TYPE);
}

Value doDiv(Value a, Value b) {
    if (IS_INTEGER(a) && IS_INTEGER(b)) {
        s64 vc = getInteger(b);
        return vc == 0 ? ERROR(E_DIV_ZERO) : VAL_INT(getInteger(a) / vc);
    } else if (IS_NUMBER(a) && IS_NUMBER(b)) {
        double vc = getDouble(b);
        return vc == 0 ? ERROR(E_DIV_ZERO) : VAL_DOUBLE(getDouble(a) / vc);
    } else {
        return ERROR(E_WRONG_TYPE);
    }
}

Value doMod(Value a, Value b) {
    return NIL;
}

Value doPow(Value a, Value b) {
    return NIL;
}

Value doAnd(Value a, Value b) {
    return IS_INTEGER(a) && IS_INTEGER(b) ? VAL_INT(getInteger(a) & getInteger(b)) : ERROR(E_WRONG_TYPE);
}

Value doOr(Value a, Value b) {
    return IS_INTEGER(a) && IS_INTEGER(b) ? VAL_INT(getInteger(a) | getInteger(b)) : ERROR(E_WRONG_TYPE);
}

Value doXor(Value a, Value b) {
    return IS_INTEGER(a) && IS_INTEGER(b) ? VAL_INT(getInteger(a) ^ getInteger(b)) : ERROR(E_WRONG_TYPE);
}

VM::VM() {
    stackSize = 512;
    stack = (Value *) calloc(stackSize, sizeof(Value));
}

VM::~VM() {
    free(stack);
    stack     = 0;
    stackSize = 0;
}

Value *VM::maybeGrowStack(Value *regs) {
    if (regs + 256 > stack + stackSize) {
        int base = regs - stack;
        stackSize += 512;
        stack = (Value *) realloc(stack, stackSize * sizeof(Value));
        return stack + base;
    }
    return regs;
}

int VM::run(unsigned *pc) {
    void *dispatch[256] = {
        &&jmp, &&call, &&return_, &&closure,
        &&move,
        &&add, &&sub, &&mul, &&div, &&mod, &&pow, &&and_, &&or_, &&xor_,
        &&end};

    {
        void *tmp[7] = {&&decodeA, &&decodeB, &&decodeAB, &&decodeC, &&decodeAC, &&decodeBC, &&decodeABC};
        void **p = dispatch+32;
        for (int i = 0; i < 7; ++i) {
            void *d = tmp[i];
            for (int j = 0; j < 32; ++j) {
                *p++ = d;
            }
        }
    }
    
    Value *regs  = stack;
    Value *ups = 0;
    Vector<RetInfo> retInfo;

    unsigned code;
    Value A, B;
    Value *ptrC;

    STEP;

 decodeAB:  DECODE(A); // fall
 decodeB:   DECODE(B); goto *dispatch[OP(code) & 0x1f];

 decodeAC:  ptrC = ups + OC(code); // fall
 decodeA:   DECODE(A); goto *dispatch[OP(code) & 0x1f];

 decodeABC: DECODE(A); // fall
 decodeBC:  DECODE(B); // fall
 decodeC:   ptrC = ups + OC(code); goto *dispatch[OP(code) & 0x1f];

 jmp: 
    assert(IS_INTEGER(A));
    if (IS_FALSE(B)) { pc += getInteger(A); }
    STEP;
    
 closure:
    assert(IS_PROTO(A));
    *ptrC = VAL_OBJ(Func::alloc((Proto *) A, ups, regs));
    STEP;

 return_:
    if (retInfo.size == 0) {
        int ret = 0;
        Value v = regs[0];
        if (IS_INTEGER(v)) {
            ret = (int) getInteger(v);
        }
        return ret;
    } else {
        RetInfo *ri = retInfo.top();
        pc   = ri->pc;
        regs = ri->regs;
        ups  = ri->ups;
        retInfo.pop();
        STEP;
    }

 call: {
        const int nEffArgs = getInteger(A);
        const Func *f = (Func *) B;
        Value *base = ptrC;

        if (TAG(A) != INTEGER || TAG(B) != OBJECT) { return 1; }

        if (f->type == FUNC) {
            Proto *proto = f->proto;
            int nArgs = f->proto->nArgs;
            bool hasEllipsis = false;
            if (nArgs < 0) {
                hasEllipsis = true;
                nArgs = -nArgs - 1;
            }
            for (Value *p=base+nEffArgs, *end=base+nArgs; p < end; ++p) {
                *p = NIL;
            }
            if (hasEllipsis) {
                if (nEffArgs < nArgs) {
                    base[nArgs-1] = EMPTY_ARRAY; // VALUE(ARRAY, 0);
                } else {
                    int nExtra = nEffArgs - nArgs;
                    Array *a = Array::alloc(nExtra);
                    for (Value *p=base+nArgs-1, *end=base+nEffArgs; p < end; ++p) {
                        a->push(*p);
                    }
                    base[nArgs-1] = VAL_OBJ(a);
                }
            }
            if (*pc == RET) {
                memmove(regs, base, nArgs * sizeof(Value));
                base = regs;
            } else {
                RetInfo *ret = retInfo.push();
                ret->pc   = pc;
                ret->regs = regs;
                ret->ups  = ups;
            }
            pc   = proto->code.buf;
            regs = maybeGrowStack(base);
            ups  = f->ups;
        } else if (f->type == CFUNC) {
            CFunc *cf = (CFunc *) A;
            cf->call(base, nEffArgs);
        } else { return 2; }
        STEP;
    }

 move: *ptrC = A; STEP;
 add:  *ptrC = doAdd(A, B); STEP;     
 sub:  *ptrC = doSub(A, B); STEP;
 mul:  *ptrC = doMul(A, B); STEP;
 div:  *ptrC = doDiv(A, B); STEP;
 mod:  *ptrC = doMod(A, B); STEP;
 pow:  *ptrC = doPow(A, B); STEP;
 and_: *ptrC = NIL;         STEP;
 or_:  *ptrC = NIL;         STEP;
 xor_: *ptrC = NIL;         STEP;
 end: return 0;
}

bool opcodeHasDest(int op) {
    return (ADD <= op && op <= LEN) || op == MOVE || op == GET;
}
