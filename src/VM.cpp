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
#include <math.h>
#include <stdio.h>

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
    Map *map = MAP(a)->copy();
    map->add(b);
    return VAL_OBJ(map);
}

Value doAdd(Value a, Value b) {
    int ta = TAG(a);
    if (IS_NUM_TAG(ta) && IS_NUM(b)) {
        return VAL_NUM(GET_NUM(a) + GET_NUM(b));
    }
    if (IS_STRING(a)) {
        return String::concat(a, b);
    }
    ERR(ta != T_OBJ, E_WRONG_TYPE);
    int type = O_TYPE(a);
    ERR(type != O_ARRAY && type != O_MAP, E_WRONG_TYPE);
    return type == O_ARRAY ? arrayAdd(a, b) : mapAdd(a, b);
}

Value doMod(Value a, Value b) {
    ERR(!IS_NUM(a) || !IS_NUM(b), E_WRONG_TYPE);
    double da = GET_NUM(a);
    double db = GET_NUM(b);
    return VAL_NUM(da - floor(da / db) * db);
}

Value doPow(Value a, Value b) {
    ERR(!IS_NUM(a) || !IS_NUM(b), E_WRONG_TYPE);
    return VAL_NUM(pow(GET_NUM(a), GET_NUM(b)));
}

static Value doSHL(Value v, int shift) {
    ERR(!IS_NUM(v), E_WRONG_TYPE);
    if (shift < 0) { shift = 0; }
    return VAL_NUM(shift >= 32 ? 0 : ((unsigned)GET_NUM(v) << shift));
}

static Value doSHR(Value v, int shift) {
    ERR(!IS_NUM(v), E_WRONG_TYPE);
    if (shift < 0) { shift = 0; }
    return VAL_NUM(shift >= 32 ? 0 : ((unsigned)GET_NUM(v) >> shift));
}

static Value indexGet(Value a, Value b) {
    return 
        IS_ARRAY(a)  ? ARRAY(a)->get(b)  :
        IS_STRING(a) ? String::get(a, b) :
        IS_MAP(a)    ? MAP(a)->get(b)    :
        ERROR(E_NOT_INDEXABLE);
}

static Value fieldGet(Value a, Value b) {
    if (IS_STRING(a)) {
        return String::methods->get(b);
    } else {
        return indexGet(a, b);
    }
}

static void indexSet(Value c, Value a, Value b) {
    ERR(IS_STRING(c), E_STRING_WRITE);
    if (IS_ARRAY(c)) {
        ARRAY(c)->set(a, b);
    } else if (IS_MAP(c)){
        MAP(c)->set(a, b);
    } else {
        ERROR(E_NOT_INDEXABLE);
    }
}

static void fieldSet(Value c, Value a, Value b) {
    indexSet(c, a, b);
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
    unsigned atype = a->type();
    if (atype == b->type()) {
        switch (atype) {
        case O_STR:    return ((String*)a)->equals((String*)b);
        case O_ARRAY:  return ((Array*)a)->equals((Array*)b);
        case O_MAP:    return ((Map*)a)->equals((Map*)b);
        }
    }
    return false;
}

static bool lessThanNonNum(Value a, Value b) {
    if (IS_STRING(a) && IS_STRING(b)) {
        char *p1 = GET_CSTR(a);
        char *p2 = GET_CSTR(b);
        unsigned len1 = len(a);
        unsigned len2 = len(b);
        int cmp = strncmp(p1, p2, min(len1, len2));
        return cmp < 0 || (cmp == 0 && len1 < len2);
    }
    if (IS_ARRAY(a) && IS_ARRAY(b)) {
        return ARRAY(a)->lessThan(ARRAY(b));
    }
    return false;
}

bool lessThan(Value a, Value b) {
    return (IS_NUM(a) && IS_NUM(b)) ? GET_NUM(a) < GET_NUM(b) : lessThanNonNum(a, b);
}

static Value constUps[N_CONST_UPS] = {
    VAL_OBJ(Map::alloc()),
    VAL_OBJ(Array::alloc()), 
    EMPTY_STRING,
    VAL_NUM(-1),
    ONE,
    ZERO,
    NIL,
};

static void copyUpvals(Func *f, Value *regs) {
    unsigned nUp = f->proto->nUp();
    unsigned nOwnUp = nUp - N_CONST_UPS;
    memcpy(regs + (256-nUp), f->ups, nOwnUp * sizeof(Value));
    memcpy(regs + (256 - N_CONST_UPS), constUps, sizeof(constUps));
}

extern __thread jmp_buf jumpBuf;
Value VM::run(Func *func, int nArg, Value *args) {
    unsigned code = 0;
    Value A, B;
    Value *ptrC;
    unsigned *pc = func->proto->code.buf();

    if (int err = setjmp(jumpBuf)) {
        (void) err;
        printf("at %d op %x\n", (int) (pc - activeFunc->proto->code.buf() - 1), code); 
        return NIL;
    }


    static void *dispatch[] = {
#define _(name) &&name
#include "opcodes.inc"
#undef _
    };

    assert(sizeof(dispatch)/sizeof(dispatch[0]) == N_OPCODES);
 
    Value *regs  = stack;
    if (nArg > 0) {
        memcpy(regs, args, nArg * sizeof(Value));
    }

    activeFunc = func;
    copyUpvals(activeFunc, regs);

    STEP;

 JMP:                          pc += OD(code);    STEP;
 JT:   if (!IS_FALSE(*ptrC)) { pc += OD(code);  } STEP;
 JF:   if ( IS_FALSE(*ptrC)) { pc += OD(code);  } STEP;
 JLT:  if (lessThan(A, B))   { pc += OSC(code); } STEP;
 JNIS: if (A != B)           { pc += OSC(code); } STEP;

 FOR: 
    A = *(ptrC + 1);
    B = *(ptrC + 2);
    ERR(!IS_NUM(A) || !IS_NUM(B), E_FOR_NOT_NUMBER);
    *ptrC = B;
    if (!(GET_NUM(B) < GET_NUM(A))) { pc += OD(code); }
    STEP;

 LOOP: {
        const double counter = GET_NUM(*ptrC) + 1;
        if (counter <  GET_NUM(*(ptrC+1))) { pc += OD(code); }
        *ptrC = VAL_NUM(counter);
        STEP;
    }

FUNC:
    assert(IS_PROTO(A));
    *ptrC = VAL_OBJ(Func::alloc(PROTO(A), regs + 256, regs, OB(code)));
    STEP;

    // index, A[B]
IGET: *ptrC = indexGet(A, B); STEP;
ISET: indexSet(*ptrC, A, B);  STEP;

    // field, A.B
FGET: *ptrC = fieldGet(A, B); STEP;
FSET: fieldSet(*ptrC, A, B);  STEP;

RET: {
        regs[0] = A;
        if (!retInfo.size()) { return A; }
        RetInfo *ri = retInfo.top();
        pc         = ri->pc;
        regs       = stack + ri->base;
        activeFunc = ri->func;
        retInfo.pop();
        copyUpvals(activeFunc, regs);
        STEP;
    }

            /*
                memmove(regs, base, nArgs * sizeof(Value));
                base = regs;
                if (activeFunc != f) {
                    copyUpvals(f, regs);
                }
            */


CALL: { 
        ERR(!IS_OBJ(A), E_CALL_NOT_FUNC);
        const int type = O_TYPE(A);
        ERR(!(type == O_FUNC || type == O_CFUNC), E_CALL_NOT_FUNC);

        const int nEffArgs = OB(code);
        Value *base = ptrC;

        if (type == O_FUNC) {
            Func *f = (Func *) GET_OBJ(A);
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
            RetInfo *ret = retInfo.push();
            ret->pc    = pc;
            ret->base  = regs - stack;
            ret->func  = activeFunc;
            regs = maybeGrowStack(base);
            copyUpvals(f, regs);
            pc   = proto->code.buf();
            activeFunc = f;
        } else { // O_CFUNC
            CFunc *cf = (CFunc *) GET_OBJ(A);
            cf->call(base, nEffArgs);
        }
        STEP;
    }

 MOVEUP: {
        const int slot = regs + 256 - ptrC;
        assert(slot > N_CONST_UPS); // E_UP_CONST);
        const int nUp = activeFunc->nUp();
        assert(slot <= nUp);
        activeFunc->ups[nUp - slot] = A;
    }
 MOVE_R: *ptrC = A; STEP;
 MOVE_I: *ptrC = VAL_NUM(OD(code)); STEP;
 MOVE_C: *ptrC = *pc | (((u64) *(pc+1)) << 32); pc += 2; STEP;
 LEN:    *ptrC = VAL_NUM(len(A)); STEP;
 NOTL:   *ptrC = IS_FALSE(A) ? TRUE : FALSE; STEP;
    // notb: *ptrC = IS_INT(A)? VAL_INT(~getInteger(A)):ERROR(E_WRONG_TYPE); STEP;

 ADD: *ptrC = doAdd(A, B); STEP;     
 SUB: *ptrC = BINOP(-, A, B); STEP;
 MUL: *ptrC = BINOP(*, A, B); STEP;
 DIV: *ptrC = BINOP(/, A, B); STEP;
 MOD: *ptrC = doMod(A, B);    STEP;
 POW: *ptrC = doPow(A, B);    STEP;

 AND: *ptrC = BITOP(&,  A, B); STEP;
 OR:  *ptrC = BITOP(|,  A, B); STEP;
 XOR: *ptrC = BITOP(^,  A, B); STEP;

 SHL_RR: ERR(!IS_NUM(B), E_WRONG_TYPE); *ptrC = doSHL(A, (int)GET_NUM(B)); STEP;
 SHR_RR: ERR(!IS_NUM(B), E_WRONG_TYPE); *ptrC = doSHR(A, (int)GET_NUM(B)); STEP;
 SHL_RI: *ptrC = doSHL(A, OSB(code));       STEP;
 SHR_RI: *ptrC = doSHR(A, OSB(code));       STEP;

 EQ:  *ptrC = equals(A, B)  ? TRUE : FALSE; STEP;
 NEQ: *ptrC = !equals(A, B) ? TRUE : FALSE; STEP;
 IS:  *ptrC = A == B ? TRUE : FALSE; STEP;
 NIS: *ptrC = A != B ? TRUE : FALSE; STEP;

 LT:  *ptrC = lessThan(A, B) ? TRUE : FALSE; STEP;
 LE:  *ptrC = (equals(A, B) || lessThan(A, B)) ? TRUE : FALSE; STEP;

    return 0;
}

bool opcodeHasDest(int op) {
    switch (op) {
    case JMP:
    case CALL:
    case RET:
    case ISET:
    case FSET:
        return false;
    }
    return true;
    // return (ADD <= op && op <= LEN) || op == MOVE || op == GET;
}
