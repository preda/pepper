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
#include "Decompile.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <setjmp.h>
#include <math.h>
#include <stdio.h>

#define FAST_CALL 0

#define DO_CALL(f, n, regs, base, stack) ({                             \
  const unsigned p1 = regs - (stack)->base, p2 = base - (stack)->base;\
  int ret = call(f, n, base, stack);\
  regs = (stack)->base + p1; base = (stack)->base + p2;\
  copyUpvals(activeFunc, regs);\
  if (ret > 0 && IS_O_TYPE(f, O_FUNC)) {\
  fprintf(stderr, "Error at bytecode #%d\n", ret-1);\
  printFunc((Func *) GET_OBJ(f));\
  } else if (ret) { fprintf(stderr, "Error %d\n", ret); }   \
  ret; })

#define STEP code=*pc++; ptrC=regs+OC(code); A=regs[OA(code)]; B=regs[OB(code)]; goto *dispatch[OP(code)];

#define INT(x) VAL_INT((signed char)x)

inline Value doAdd(GC *gc, Value a, Value b) {
    int ta = TAG(a);
    if (IS_NUM_TAG(ta) && IS_NUM(b)) {
        return VAL_NUM(GET_NUM(a) + GET_NUM(b));
    } else if (IS_STRING(a)) {
        return String::concat(gc, a, b);
    } else {
        if (ta != T_OBJ) { return E_WRONG_TYPE; }
        int type = O_TYPE(a);
        if (type == O_ARRAY && (IS_ARRAY(b) || IS_MAP(b) || IS_STRING(b))) {
            Array *array = Array::alloc(gc);
            array->add(a);
            array->add(b);
            return VAL_OBJ(array);
        } else if (type == O_MAP && (IS_ARRAY(b) || IS_MAP(b) || IS_STRING(b))) {
            Map *map = MAP(a)->copy(gc);
            map->add(b);
            return VAL_OBJ(map);
        } else {
            return VERR;
        }
    }
}

Value doMod(Value a, Value b) {
    if (!IS_NUM(a) || !IS_NUM(b)) { return VERR; }
    double da = GET_NUM(a);
    double db = GET_NUM(b);
    return VAL_NUM(da - floor(da / db) * db);
}

Value doPow(Value a, Value b) {
    if(!IS_NUM(a) || !IS_NUM(b)) { return VERR; }
    return VAL_NUM(pow(GET_NUM(a), GET_NUM(b)));
}

static Value doSHL(Value v, int shift) {
    if (!IS_NUM(v)) { return VERR; }
    if (shift < 0) { shift = 0; }
    return VAL_NUM(shift >= 32 ? 0 : ((unsigned)GET_NUM(v) << shift));
}

static Value doSHR(Value v, int shift) {
    if (!IS_NUM(v)) { return VERR; }
    if (shift < 0) { shift = 0; }
    return VAL_NUM(shift >= 32 ? 0 : ((unsigned)GET_NUM(v) >> shift));
}

static Value getIndex(Value a, Value key) {
    return IS_ARRAY(a)  ? ARRAY(a)->getV(key) :
        IS_STRING(a) ? String::get(a, key) :
        IS_MAP(a)    ? MAP(a)->rawGet(key) :
        VERR;
    // E_NOT_INDEXABLE);
}

static bool acceptsIndex(Value v) {
    return IS_MAP(v) || IS_ARRAY(v) || IS_STRING(v);
}

static int setIndex(Value c, Value a, Value b) {
    if (IS_ARRAY(c)) {
        ARRAY(c)->setV(a, b);
    } else if (IS_MAP(c)) {
        bool again = false;
        MAP(c)->set(a, b, &again);
    } else {
        return IS_STRING(c) ? E_STRING_WRITE : E_NOT_INDEXABLE;
    }
    return 0;
}

static int setField(Value c, Value a, Value b) {
    return setIndex(c, a, b);
}

static Value getSlice(GC *gc, Value a, Value b1, Value b2) {
    return 
        IS_ARRAY(a)  ? ARRAY(a)->getSliceV(gc, b1, b2) :
        IS_STRING(a) ? String::getSlice(gc, a, b1, b2) :
        VERR;
}

static int setSlice(Value c, Value a1, Value a2, Value b) {
    if (IS_ARRAY(c)) {
        ARRAY(c)->setSliceV(a1, a2, b);
        return 0;
    } else {
        return E_NOT_SLICEABLE;
    }
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

static void copyUpvals(Func *f, Value *regs) {
    unsigned nUp = f->proto->nUp();
    // unsigned nOwnUp = nUp - N_CONST_UPS;
    memcpy(regs + (256-nUp), f->ups, nUp * sizeof(Value));
    // memcpy(regs + (256 - N_CONST_UPS), constUps, sizeof(constUps));
}

Value VM::run(Func *activeFunc, int nArg, Value *args) {
    static const int nExtra = 2;
    Stack stack;
    Value *regs = stack.maybeGrow(0, nArg + nExtra + 1);
    regs[0] = VAL_OBJ(activeFunc);
    regs[1] = stringMethods;
    Value *base = regs + nExtra;
    base[0] = VNIL;
    memcpy(base+1, args, nArg * sizeof(Value));
    Value fv = VAL_OBJ(activeFunc);
    int err = DO_CALL(fv, nArg, regs, base, &stack);    
    return !err ? base[0] : VNIL;
}

#define NARGS_CFUNC 128
static int prepareStackForCall(Value *base, int nArgs, int nEffArgs, GC *gc) {
    Array *tail  = 0;
    int tailSize = 0;
    if (nEffArgs < 0) { //last arg is *args
        nEffArgs = -nEffArgs;
        Value v = base[nEffArgs - 1];
        if (!IS_ARRAY(v)) {
            fprintf(stderr, "*args in call is not Array");
            __builtin_abort();
        }
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

// extern __thread jmp_buf jumpBuf;
int VM::call(Value A, int nEffArgs, Value *regs, Stack *stack) {
    Vector<RetInfo> retInfo; // only used if FAST_CALL

    if (!(IS_O_TYPE(A, O_FUNC) || IS_CF(A) || IS_O_TYPE(A, O_CFUNC))) { return -1; }
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
        return 0;
    }

    unsigned code = 0;
    Value B;
    Value *ptrC;
    Func *activeFunc = (Func *) GET_OBJ(A);
    unsigned *pc = (unsigned *) activeFunc->proto->code.buf();

    /*
    if (int err = setjmp(jumpBuf)) {
        (void) err;
        printf("at %d op %x\n", (int) (pc - activeFunc->proto->code.buf() - 1), code);
        printFunc(activeFunc);
        __builtin_abort();
        return;
    }
    */

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
    if (!IS_NUM(A) || !IS_NUM(B)) { goto error; } // E_FOR_NOT_NUMBER
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
 GETI: *ptrC = getIndex(A, B); if (*ptrC == VERR) { goto error; } STEP;
 SETI: if (setIndex(*ptrC, A, B)) { goto error; } STEP;

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
                const int oa = OA(code);
                const int ob = OB(code);
                int top = max(oa, ob) + 1;
                top = max(top, activeFunc->proto->localsTop);
                Value *base = regs + top;
                printf("top %d\n", top);
                base[0] = A;
                base[1] = B;
                int cPos = ptrC - regs;
                DO_CALL(v, 2, regs, base, stack);
                regs[cPos] = base[0];
                break;
            }
        }
    }
    if (*ptrC == VERR) { goto error; }
    STEP;

 SETF: if (setField(*ptrC, A, B)) { goto error; } STEP;

 GETS: *ptrC = getSlice(gc, A, B, regs[OB(code)+1]); if (*ptrC==VERR) { goto error; } STEP;
 SETS: if (setSlice(*ptrC, A, regs[OA(code)+1], B)) { goto error; } STEP;

 RET: {
        regs[0] = A;
        Value *root = stack->base;
        gc->maybeCollect(this, root, regs - root + 1);
#if FAST_CALL
        if (!retInfo.size()) {
            return 0;
        }
        RetInfo *ri = retInfo.top();
        pc         = ri->pc;
        regs       = stack->base + ri->base;
        activeFunc = ri->func;
        retInfo.pop();
        copyUpvals(activeFunc, regs);
        STEP;
#else
        return 0;
#endif
    }

CALL: { 
        if (!IS_OBJ(A) && !IS_CF(A)) { goto error; } // E_CALL_NOT_FUNC
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
            int ret = DO_CALL(A, nEffArgs, regs, base, stack);
            if (ret) { goto error; }
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
 MOVE_C: {
        Value v = *pc | (((u64) *(pc+1)) << 32);
        pc += 2;
        if (IS_ARRAY(v)) {
            v = VAL_OBJ(ARRAY(v)->copy(gc));
        } else if (IS_MAP(v)) {
            v = VAL_OBJ(MAP(v)->copy(gc));
        }
        *ptrC = v;
        STEP;
    }
 LEN:    *ptrC = VAL_NUM(len(A)); STEP;
 NOTL:   *ptrC = IS_FALSE(A) ? TRUE : FALSE; STEP;
    // notb: *ptrC = IS_INT(A)? VAL_INT(~getInteger(A)):ERROR(E_WRONG_TYPE); STEP;

 ADD: *ptrC = doAdd(gc, A, B); if (*ptrC == VERR) { goto error; } STEP;
 SUB: *ptrC = BINOP(-, A, B); STEP;
 MUL: *ptrC = BINOP(*, A, B); STEP;
 DIV: *ptrC = BINOP(/, A, B); STEP;
 MOD: *ptrC = doMod(A, B); if (*ptrC == VERR) { goto error; } STEP;
 POW: *ptrC = doPow(A, B); if (*ptrC == VERR) { goto error; } STEP;

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

 error: return pc - (unsigned *) activeFunc->proto->code.buf();
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
