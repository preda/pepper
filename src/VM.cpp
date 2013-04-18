#include "VM.h"
#include "Array.h"
#include "Map.h"
#include "String.h"
#include "Func.h"
#include "CFunc.h"
#include "Proto.h"
#include "Value.h"
#include "Object.h"
#include "RetInfo.h"
#include "GC.h"
#include "NameValue.h"
#include "Stack.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <setjmp.h>
#include <math.h>
#include <stdio.h>

#define FAST_CALL 0

#define DO_CALL(f, n, regs, base, stack) {\
  unsigned p1 = regs - (stack)->base, p2 = base - (stack)->base;\
  call(f, n, base, stack);\
  regs = (stack)->base + p1; base = (stack)->base + p2;\
  copyUpvals(activeFunc, regs);\
}

#define STEP code=*pc++; ptrC=regs+OC(code); A=regs[OA(code)]; B=regs[OB(code)]; goto *dispatch[OP(code)];

#define INT(x) VAL_INT((signed char)x)

static Value arrayAdd(GC *gc, Value a, Value b) {
    assert(IS_ARRAY(a));
    ERR(!(IS_ARRAY(b) || IS_MAP(b) || IS_STRING(b)), E_WRONG_TYPE);
    Array *array = Array::alloc(gc);
    array->add(a);
    array->add(b);
    return VAL_OBJ(array);
}

static Value mapAdd(GC *gc, Value a, Value b) {
    assert(IS_MAP(a));
    ERR(!(IS_ARRAY(b) || IS_MAP(b) || IS_STRING(b)), E_WRONG_TYPE);
    Map *map = MAP(a)->copy(gc);
    map->add(b);
    return VAL_OBJ(map);
}

Value doAdd(GC *gc, Value a, Value b) {
    int ta = TAG(a);
    if (IS_NUM_TAG(ta) && IS_NUM(b)) {
        return VAL_NUM(GET_NUM(a) + GET_NUM(b));
    }
    if (IS_STRING(a)) {
        return String::concat(gc, a, b);
    }
    ERR(ta != T_OBJ, E_WRONG_TYPE);
    int type = O_TYPE(a);
    ERR(type != O_ARRAY && type != O_MAP, E_WRONG_TYPE);
    return type == O_ARRAY ? arrayAdd(gc, a, b) : mapAdd(gc, a, b);
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

static Value getIndex(Value a, Value key) {
    return 
        IS_ARRAY(a)  ? ARRAY(a)->getV(key) :
        IS_STRING(a) ? String::get(a, key) :
        IS_MAP(a)    ? MAP(a)->rawGet(key) :
        ERROR(E_NOT_INDEXABLE);
}

static bool acceptsIndex(Value v) {
    return IS_MAP(v) || IS_ARRAY(v) || IS_STRING(v);
}
/*
static Value getField(Value a, Value key, bool *again) {
    while (true) {
        if (IS_MAP(a)) {
            Map *map = MAP(a);
            bool again = false;
            Value v = map->get(key, &again);
            if (again) {
                if (acceptsIndex(v)) {
                    a = v;
                } else {
                    return v;
                    // Value args[2] = {a, key};
                    // call(v, 2, args, stack);
                }
            } else {
                return v;
            }
        } else {
            return getIndex(a, key);
        }
    }
}
*/

static void setIndex(Value c, Value a, Value b) {
    ERR(IS_STRING(c), E_STRING_WRITE);
    if (IS_ARRAY(c)) {
        ARRAY(c)->setV(a, b);
    } else if (IS_MAP(c)) {
        bool again = false;
        MAP(c)->set(a, b, &again);
    } else {
        ERROR(E_NOT_INDEXABLE);
    }
}

static void setField(Value c, Value a, Value b) {
    setIndex(c, a, b);
}

static Value getSlice(GC *gc, Value a, Value b1, Value b2) {
    return 
        IS_ARRAY(a)  ? ARRAY(a)->getSliceV(gc, b1, b2) :
        IS_STRING(a) ? String::getSlice(gc, a, b1, b2) :
        ERROR(E_NOT_SLICEABLE);
}

static void setSlice(Value c, Value a1, Value a2, Value b) {
    ERR(IS_STRING(c), E_STRING_WRITE);
    ERR(!IS_ARRAY(c), E_NOT_SLICEABLE);
    ARRAY(c)->setSliceV(a1, a2, b);
}

VM::VM(GC *gc, void *context) :
    gc(gc),
    context(context)
{
    stringMethods = Map::makeMap(gc, "find", CFunc::value(gc, String::method_find), NULL);
}

VM::~VM() {
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

/*
static Value constUps[N_CONST_UPS] = {
    VAL_OBJ(Map::alloc()),
    VAL_OBJ(Array::alloc()), 
    EMPTY_STRING,
    VAL_NUM(-1),
    ONE,
    ZERO,
    NIL,
};
*/

static void copyUpvals(Func *f, Value *regs) {
    unsigned nUp = f->proto->nUp();
    // unsigned nOwnUp = nUp - N_CONST_UPS;
    memcpy(regs + (256-nUp), f->ups, nUp * sizeof(Value));
    // memcpy(regs + (256 - N_CONST_UPS), constUps, sizeof(constUps));
}

Value VM::run(Func *f, int nArg, Value *args) {
    static const int nExtra = 2;
    Stack stack;
    Value *regs = stack.maybeGrow(0, nArg + nExtra + 1);
    regs[0] = VAL_OBJ(f);
    regs[1] = stringMethods;
    Value *base = regs + nExtra;
    base[0] = VNIL;
    memcpy(base+1, args, nArg * sizeof(Value));
    Value fv = VAL_OBJ(f);
    Func *activeFunc = f;
    DO_CALL(fv, nArg, regs, base, &stack);
    return base[0];
}

#define NARGS_CFUNC 128
static int prepareStackForCall(Value *base, int nArgs, int nEffArgs, GC *gc) {
    Array *tail  = 0;
    int tailSize = 0;
    if (nEffArgs < 0) { //last arg is *args
        nEffArgs = -nEffArgs;
        Value v = base[nEffArgs - 1];
        ERR(!IS_ARRAY(v), E_ELLIPSIS);
        tail = ARRAY(v);
        --nEffArgs;
        tailSize = tail->size();
    }

    bool hasEllipsis = nArgs < 0;
    if (hasEllipsis) {
        nArgs = -nArgs - 1;
    }
    int nFromTail  = clamp(nArgs - nEffArgs, 0, tailSize);
    for (int i = 0; i < nFromTail; ++i) {
        base[nEffArgs + i] = tail->getI(i);
    }
    nEffArgs += nFromTail;
    if (nArgs != NARGS_CFUNC) {
        for (Value *p = base + nEffArgs, *end = base + nArgs; p < end; ++p) {
            *p = VNIL;
        }
    }
    if (hasEllipsis) {
        Array *a = Array::alloc(gc);
        for (Value *p = base + nArgs, *end = base + nEffArgs; p < end; ++p) {
            a->push(*p);
        }
        if (nFromTail < tailSize) {
            a->append(tail->buf() + nFromTail, tailSize - nFromTail);
        }
        base[nArgs] = VAL_OBJ(a);
    }
    return nEffArgs;
}

extern __thread jmp_buf jumpBuf;
void VM::call(Value A, int nEffArgs, Value *regs, Stack *stack) {
    Vector<RetInfo> retInfo; // only used if FAST_CALL

    ERR(!(IS_O_TYPE(A, O_FUNC) || IS_CF(A) || IS_O_TYPE(A, O_CFUNC)), E_CALL_NOT_FUNC);
    regs  = stack->maybeGrow(regs, 256);
    int nExpectedArgs = IS_O_TYPE(A, O_FUNC) ? ((Func *)GET_OBJ(A))->proto->nArgs : NARGS_CFUNC;
    nEffArgs = prepareStackForCall(regs, nExpectedArgs, nEffArgs, gc);

    if (IS_CF(A) || IS_O_TYPE(A, O_CFUNC)) {
        if (IS_CF(A)) {
            tfunc f = GET_CF(A);
            *regs = f(this, CFunc::CFUNC_CALL, 0, regs, nEffArgs);
        } else {
            ((CFunc *) GET_OBJ(A))->call(this, regs, nEffArgs);
        }
        return; //regs;
    }

    unsigned code = 0;
    Value B;
    Value *ptrC;
    Func *activeFunc = (Func *) GET_OBJ(A);
    unsigned *pc = activeFunc->proto->code.buf();

    if (int err = setjmp(jumpBuf)) {
        (void) err;
        printf("at %d op %x\n", (int) (pc - activeFunc->proto->code.buf() - 1), code); 
        return; // 0;
    }

    static void *dispatch[] = {
#define _(name) &&name
#include "opcodes.inc"
#undef _
    };

    assert(sizeof(dispatch)/sizeof(dispatch[0]) == N_OPCODES);

    copyUpvals(activeFunc, regs);

    STEP;

 JMP:  pc += OD(code);    STEP;
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
        if (counter < GET_NUM(*(ptrC+1))) { pc += OD(code); }
        *ptrC = VAL_NUM(counter);
        STEP;
    }

FUNC:
    assert(IS_PROTO(A));
    *ptrC = VAL_OBJ(Func::alloc(gc, PROTO(A), regs + 256, regs, OB(code)));
    STEP;

    // index, A[B]
GETI: *ptrC = getIndex(A, B); STEP;
SETI: setIndex(*ptrC, A, B);  STEP;

    // field, A.B
 GETF: while (true) {
        if (IS_STRING(A)) {
            A = stringMethods;
        }
        if (!IS_MAP(A)) {
            *ptrC = getIndex(A, B);
            break;
        } else {
            Map *map = MAP(A);
            bool recurse = false;
            Value v = map->get(B, &recurse);
            if (!recurse) {
                *ptrC = v;
                break;
            } else if (acceptsIndex(v)) {
                A = v;
            } else {
                Value *base = regs + activeFunc->proto->localsTop;
                base[0] = A;
                base[1] = B;
                int cPos = ptrC - regs;
                DO_CALL(v, 2, regs, base, stack);
                regs[cPos] = base[0];
                break;
            }
        }
    }
    STEP;

 SETF: setField(*ptrC, A, B);  STEP;

 GETS: *ptrC = getSlice(gc, A, B, regs[OB(code)+1]); STEP;
 SETS: setSlice(*ptrC, A, regs[OA(code)+1], B);  STEP;

 RET: {
        regs[0] = A;
        Value *root = stack->base;
        gc->maybeCollect(this, root, regs - root + 1);
#if FAST_CALL
        if (!retInfo.size()) {
            return;
        }
        RetInfo *ri = retInfo.top();
        pc         = ri->pc;
        regs       = stack->base + ri->base;
        activeFunc = ri->func;
        retInfo.pop();
        copyUpvals(activeFunc, regs);
        STEP;
#else
        return;
#endif
    }

CALL: { 
        ERR(!IS_OBJ(A) && !IS_CF(A), E_CALL_NOT_FUNC);
        int nEffArgs = OSB(code);
        assert(nEffArgs != 0);
        Value *base = ptrC;
#if FAST_CALL
        if (IS_O_TYPE(A, O_FUNC)) {
            Func *f = (Func *) GET_OBJ(A);
            Proto *proto = f->proto;
            prepareStackForCall(base, proto->nArgs, nEffArgs, gc);
            RetInfo *ret = retInfo.push();
            ret->pc    = pc;
            ret->base  = regs - stack->base;
            ret->func  = activeFunc;
            regs = stack->maybeGrow(base, 256);
            copyUpvals(f, regs);
            pc   = proto->code.buf();
            activeFunc = f;
        } else {
#endif
            DO_CALL(A, nEffArgs, regs, base, stack);
            /*
            unsigned regPos = regs - stack->base;
            call(A, nEffArgs, base, stack);
            regs = stack->base + regPos;
            copyUpvals(activeFunc, regs);
            */
#if FAST_CALL
        }
#endif
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

 ADD: *ptrC = doAdd(gc, A, B); STEP;
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

    return; //0
}

bool opcodeHasDest(int op) {
    switch (op) {
    case JMP:
    case CALL:
    case RET:
    case SETI:
    case SETF:
        return false;
    }
    return true;
    // return (ADD <= op && op <= LEN) || op == MOVE || op == GET;
}
