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
#include <setjmp.h>

#define STEP code=*pc++; ptrC=regs+OC(code); A=regs[OA(code)]; B=regs[OB(code)]; goto *dispatch[OP(code)];

#define INT(x) VAL_INT((signed char)x)

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

Value doGet(Value a, Value b) {
    return 
        IS_ARRAY(a)  ? ((Array*)a)->get(b)  :
        IS_STRING(a) ? String::get(a, b) :
        IS_MAP(a)    ? ((Map*)a)->get(b)    :
        ERROR(E_NOT_INDEXABLE);
}

void doSet(Value c, Value a, Value b) {
    ERR(IS_STRING(c), E_STRING_WRITE);
    if (IS_ARRAY(c)) {
        // ERR(!IS_INT(a), E_INDEX_NOT_INT);
        ((Array *) c)->set(a, b);
    } else if (IS_MAP(c)){
        ((Map *) c)->set(a, b);
    } else {
        ERROR(E_NOT_INDEXABLE);
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

bool objEquals(Object *a, Object *b) {
    if (a->type == b->type) {
        switch (a->type) {
        case O_STR: return ((String *) a)->equals((String *) b);
        case O_ARRAY:  return ((Array *) a)->equals((Array *) b);
        case O_MAP:    return ((Map *) a)->equals((Map *) b);
        }
    }
    return false;
}

static void copyUpvals(Func *f, Value *regs) {
    int n = f->proto->ups.size;
    memcpy(regs + (256-n), f->ups, n * sizeof(Value));
}

extern __thread jmp_buf jumpBuf;
Value VM::run(Func *f) {
    if (int err = setjmp(jumpBuf)) {
        (void) err;
        return NIL;
    }


    static void *dispatch[] = {
#define _(name) &&name
#include "opcodes.inc"
#undef _
    };
    /*
        &&jmp, &&jmpf, &&jmpt,
        &&call, &&retur, &&func, &&get, &&set,
        &&moveup, &&move_r, &&move_i, &&move_c, 
        &&len, &&notl,
        &&add, &&sub, &&mul, &&div, &&mod, &&pow,
        &&andb, &&orb, &&xorb, &&shl_rr, &&shl_ri, &&shr_rr, &&shr_ri,
        &&eq, &&neq, &&lt, &&le
    */

    assert(sizeof(dispatch)/sizeof(dispatch[0]) == N_OPCODES);
 
    Value *regs  = stack;
    activeFunc = f;
    copyUpvals(activeFunc, regs);
    unsigned *pc = f->proto->code.buf;

    unsigned code;
    Value A, B;
    Value *ptrC;

    STEP;

 JMP:                          pc += OD(code);    STEP;
 JT:   if (!IS_FALSE(*ptrC)) { pc += OD(code);  } STEP;
 JF:   if ( IS_FALSE(*ptrC)) { pc += OD(code);  } STEP;
 JLT:  if (lessThan(A, B))   { pc += OSC(code); } STEP;
 JNIS: if (A != B)           { pc += OSC(code); } STEP;

FUNC:
    assert(IS_PROTO(A));
    *ptrC = VAL_OBJ(Func::alloc((Proto *) A, regs + 256 - activeFunc->proto->ups.size, regs));
    STEP;

GET: *ptrC = doGet(A, B); STEP;
SET: doSet(*ptrC, A, B);  STEP;

RET: {
        regs[0] = A;
        if (retInfo.size == 0) { return A; }
        RetInfo *ri = retInfo.top();
        pc         = ri->pc;
        regs       = ri->regs;
        activeFunc = ri->func;
        retInfo.pop();
        copyUpvals(activeFunc, regs);
        STEP;
    }

CALL: { 
        ERR(TAG(A) != T_OBJ || A == NIL, E_CALL_NIL);
        const int nEffArgs = OB(code);
        Value *base = ptrC;
        int type = O_TYPE(A);
        assert(type == O_FUNC || type == O_CFUNC);
        if (type == O_FUNC) {
            Func *f = (Func *) A;
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
                ret->pc    = pc;
                ret->regs  = regs;
                ret->func  = activeFunc;
            }
            pc   = proto->code.buf;
            regs = maybeGrowStack(base);
            activeFunc = f;
            copyUpvals(f, regs);
        } else { // O_CFUNC
            CFunc *cf = (CFunc *) A;
            cf->call(base, nEffArgs);
        }
        STEP;
    }

 MOVEUP: activeFunc->ups[activeFunc->proto->ups.size + (ptrC-regs) - 256] = A;
 MOVE_R: *ptrC = A; STEP;
 MOVE_I: *ptrC = VAL_INT(OD(code)); STEP;
 MOVE_C: *ptrC = *pc | (((u64) *(pc+1)) << 32); pc += 2; STEP;
 LEN:    *ptrC = VAL_INT(len(A)); STEP;
 NOTL:   *ptrC = IS_FALSE(A) ? TRUE : FALSE; STEP;
    // notb: *ptrC = IS_INT(A)? VAL_INT(~getInteger(A)):ERROR(E_WRONG_TYPE); STEP;

 ADD: *ptrC = IS_INT(A) && IS_INT(B) ? VAL_INT(getInteger(A) + getInteger(B)) : doAdd(A, B); STEP;     
 SUB: *ptrC = doSub(A, B); STEP;
 MUL: *ptrC = doMul(A, B); STEP;
 DIV: *ptrC = doDiv(A, B); STEP;
 MOD: *ptrC = doMod(A, B); STEP;
 POW: *ptrC = doPow(A, B); STEP;

#define BITOP(op, A, B) IS_INT(A) && IS_INT(B) ? VAL_INT(getInteger(A) op getInteger(B)) : ERROR(E_WRONG_TYPE)
 AND: *ptrC = BITOP(&,  A, B); STEP;
 OR:  *ptrC = BITOP(|,  A, B); STEP;
 XOR: *ptrC = BITOP(^,  A, B); STEP;

 SHL_RR: *ptrC = BITOP(<<, A, B); STEP;
 SHL_RI: *ptrC = IS_INT(A) ? VAL_INT(getInteger(A) << OB(code)) : ERROR(E_WRONG_TYPE); STEP;
 SHR_RR: *ptrC = BITOP(>>, A, B); STEP;
 SHR_RI: *ptrC = IS_INT(A) ? VAL_INT(getInteger(A) >> OB(code)) : ERROR(E_WRONG_TYPE); STEP;

 EQ:  *ptrC = equals(A, B)  ? TRUE : FALSE; STEP;
 NEQ: *ptrC = !equals(A, B) ? TRUE : FALSE; STEP;
 LT:  *ptrC = lessThan(A, B) ? TRUE : FALSE; STEP;
 LE:  *ptrC = A==B || equals(A, B) || lessThan(A, B) ? TRUE : FALSE; STEP;

    return 0;
}

bool opcodeHasDest(int op) {
    switch (op) {
    case JMP:
    case CALL:
    case RET:
    case SET:
        return false;
    }
    return true;
    // return (ADD <= op && op <= LEN) || op == MOVE || op == GET;
}

bool lessThan(Value a, Value b) {
    if (IS_INT(a) && IS_INT(b)) {
        return getInteger(a) < getInteger(b);
    }
    if (IS_NUMBER(a) && IS_NUMBER(b)) {
        return getDouble(a) < getDouble(b);
    }
    if (IS_STRING(a) && IS_STRING(b)) {
        char *p1 = GET_CSTR(a);
        char *p2 = GET_CSTR(b);
        unsigned len1 = len(a);
        unsigned len2 = len(b);
        int cmp = strncmp(p1, p2, min(len1, len2));
        return cmp < 0 || (cmp == 0 && len1 < len2);
    }
    if (IS_ARRAY(a) && IS_ARRAY(b)) {
        return ((Array *) a)->lessThan((Array *) b);
    }
    return false;
}
