#include "VM.h"
#include "Array.h"
#include "Map.h"
#include "String.h"
#include "Func.h"
#include "CFunc.h"
#include "Proto.h"
#include "Value.h"
#include "Object.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define STEP code=*pc++; ptrC=regs+OC(code); A=regs[OA(code)]; B=regs[OB(code)]; goto *dispatch[OP(code)]

#define INT(x) VAL_INT((signed char)x)

#define DECODE(A) { byte v=O##A(code); A=v<0x80 ? VAL_INT((((signed char)(v<<1))>>1)) : v<0xf0 ? ups[v & 0x7f] : VALUE((v&0xf), 0); }

static Value arrayAdd(Value a, Value b) {
    assert(IS_ARRAY(a));
    ERR(!(IS_ARRAY(b) || IS_MAP(b) || IS_STRING(b)), E_WRONG_TYPE);
    Array *array = Array::alloc();
    array->add(a);
    array->add(b);
    return VAL_OBJ(array);
}

static Value mapAdd(Value a, Value b) {
    assert(IS_MAP(a));
    ERR(!(IS_ARRAY(b) || IS_MAP(b) || IS_STRING(b)), E_WRONG_TYPE);
    Map *map = ((Map *) a)->copy();
    map->add(b);
    return VAL_OBJ(map);
}

Value doAdd(Value a, Value b) {
    return 
        IS_INT(a) && IS_INT(b) ? VAL_INT(getInteger(a) + getInteger(b))  :
        IS_NUMBER(a)  && IS_NUMBER(b)  ? VAL_DOUBLE(getDouble(a) + getDouble(b)) :
        IS_STRING(a) ? String::concat(a, b) :
        IS_ARRAY(a)  ? arrayAdd(a, b) :
        IS_MAP(a)    ? mapAdd(a, b) :
        ERROR(E_WRONG_TYPE);
}

Value doSub(Value a, Value b) {
    return 
        IS_INT(a) && IS_INT(b) ? VAL_INT(getInteger(a) - getInteger(b))  :
        IS_NUMBER(a)  && IS_NUMBER(b)  ? VAL_DOUBLE(getDouble(a) - getDouble(b)) :
        ERROR(E_WRONG_TYPE);
}

Value doMul(Value a, Value b) {
    return 
        IS_INT(a) && IS_INT(b) ? VAL_INT(getInteger(a) * getInteger(b))  :
        IS_NUMBER(a)  && IS_NUMBER(b)  ? VAL_DOUBLE(getDouble(a) * getDouble(b)) :
        ERROR(E_WRONG_TYPE);
}

Value doDiv(Value a, Value b) {
    if (IS_INT(a) && IS_INT(b)) {
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
    return IS_INT(a) && IS_INT(b) ? VAL_INT(getInteger(a) & getInteger(b)) : ERROR(E_WRONG_TYPE);
}

Value doOr(Value a, Value b) {
    return IS_INT(a) && IS_INT(b) ? VAL_INT(getInteger(a) | getInteger(b)) : ERROR(E_WRONG_TYPE);
}

Value doXor(Value a, Value b) {
    return IS_INT(a) && IS_INT(b) ? VAL_INT(getInteger(a) ^ getInteger(b)) : ERROR(E_WRONG_TYPE);
}

Value doGet(Value a, Value b) {
    return 
        IS_ARRAY(a)  ? Array::get(a, b)  :
        IS_STRING(a) ? String::get(a, b) :
        IS_MAP(a)    ? Map::get(a, b)    :
        ERROR(E_NOT_INDEXABLE);
}

void doSet(Value c, Value a, Value b) {
    ERR(IS_STRING(c), E_STRING_WRITE);
    if (IS_ARRAY(c)) {

        
    }
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

#define _32TIMES(a) a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a

Value VM::run(Func *f) {
    static void *dispatch[] = {
        &&jmp, &&call, &&return_, &&func,
        &&get, &&set, &&move, &&len,
        &&add, &&sub, &&mul, &&div, 
        &&mod, &&pow,
        &&and_, &&or_, &&xor_, 
        &&lsl, &&lsr, &&asr, &&not_,
        &&eq, &&lt, &&le,
        0, 0, 0, 0, 0, 0, 0, 0,
        _32TIMES(&&decA),
        _32TIMES(&&decB),
        _32TIMES(&&decAB),
        _32TIMES(&&decC),
        _32TIMES(&&decAC),
        _32TIMES(&&decBC),
        _32TIMES(&&decABC),
    };
    assert(sizeof(dispatch)/sizeof(dispatch[0]) == 256);
 
    Value *regs  = stack;
    Value *ups   = f->ups;
    unsigned *pc = f->proto->code.buf;

    unsigned code;
    Value A, B;
    Value *ptrC;

    STEP;

 decAB:  DECODE(A); // fall
 decB:   DECODE(B); goto *dispatch[OP(code) & 0x1f];

 decAC:  ptrC = ups + OC(code); // fall
 decA:   DECODE(A); goto *dispatch[OP(code) & 0x1f];

 decABC: DECODE(A); // fall
 decBC:  DECODE(B); // fall
 decC:   ptrC = ups + OC(code); goto *dispatch[OP(code) & 0x1f];

 jmp: 
    assert(IS_INT(A));
    if (IS_FALSE(B)) { pc += getInteger(A); }
    STEP;
    
 func:
    assert(IS_PROTO(A));
    *ptrC = VAL_OBJ(Func::alloc((Proto *) A, ups, regs));
    STEP;

 get: *ptrC = doGet(A, B); STEP;    
 set: doSet(*ptrC, A, B); STEP;

 return_: {
        regs[0] = A;
        if (retInfo.size == 0) { return A; }
        RetInfo *ri = retInfo.top();
        pc   = ri->pc;
        regs = ri->regs;
        ups  = ri->ups;
        retInfo.pop();
        STEP;
    }

 call: {
        assert(IS_INT(A));
        assert(TAG(B)==T_OBJ && B != NIL);

        const int nEffArgs = getInteger(A);
        Value *base = ptrC;
        int type = O_TYPE(B);
        assert(type == O_FUNC || type == O_CFUNC);
        if (type == O_FUNC) {
            const Func *f = (Func *) B;
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
                    base[nArgs-1] = VAL_OBJ(Array::alloc());
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
        } else { // O_CFUNC
            CFunc *cf = (CFunc *) B;
            cf->call(base, nEffArgs);
        }
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
 lsl:  *ptrC = NIL; STEP;
 lsr:  *ptrC = NIL; STEP;
 asr:  *ptrC = NIL; STEP;

 not_: *ptrC = NIL; STEP;
 
 eq:   *ptrC = equals(A, B) ? TRUE : FALSE; STEP;

 lt:   *ptrC = NIL; STEP;
 le:   *ptrC = NIL; STEP;
 len:  *ptrC = len(A); STEP;

    return 0;
}

bool opcodeHasDest(int op) {
    return (ADD <= op && op <= LEN) || op == MOVE || op == GET;
}

bool equals(Value a, Value b) {
    if (a == b) { return true; }
    if (TAG(a) == T_OBJ && TAG(b) == T_OBJ && O_TYPE(a) == O_TYPE(b)) {
        switch (O_TYPE(a)) {
        case O_STR: return ((String *) a)->equals((String *) b);
        case O_ARRAY:  return ((Array *) a)->equals((Array *) b);
        case O_MAP:    return ((Map *) a)->equals((Map *) b);
        }
    }
    return false;
}
