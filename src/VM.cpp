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

#define STEP code=*pc++; ptrC=regs+OC(code); A=regs[OA(code)]; goto *dispatch[OP(code)]

#define INT(x) VAL_INT((signed char)x)

#define DECODE(A) { byte v=O##A(code); A=v<0x80 ? VAL_INT((((signed char)(v<<1))>>1)) : ups[v & 0x7f]; }
#define DECODEC ptrC = ups + OC(code)

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

extern __thread jmp_buf jumpBuf;
Value VM::run(Func *f) {
    if (int err = setjmp(jumpBuf)) {
        (void) err;
        return NIL;
    }

#define LABEL(L) L##101: DECODEC; L##100: DECODE(A); L##000

#define LABEL2(L) L##001: DECODEC; goto L##000;\
 L##111: DECODEC; L##110: DECODE(A); L##010: DECODE(B); goto L;          \
 L##011: DECODEC; DECODE(B); goto L;

#define OPCODES(F) &&jmp##F, &&call##F, &&retur##F, &&func##F,\
&&get##F, &&set##F, &&move##F, &&len##F, \
&&add##F, &&sub##F, &&mul##F, &&div##F, &&mod##F, &&pow##F,\
&&andb##F, &&orb##F, &&xorb##F, &&notb##F, &&shl##F, &&shr##F, &&notl##F,\
&&eq##F, &&neq##F, &&lt##F, &&le##F, 0, 0, 0, 0, 0, 0, 0

    static void *dispatch[] = {
        OPCODES(000), OPCODES(100), OPCODES(010), OPCODES(110),
        OPCODES(001), OPCODES(101), OPCODES(011), OPCODES(111),
    };
    assert(sizeof(dispatch)/sizeof(dispatch[0]) == 256);
 
    Value *regs  = stack;
    Value *ups   = f->ups;
    unsigned *pc = f->proto->code.buf;

    unsigned code;
    Value A, B;
    Value *ptrC;

    STEP;


 jmp110: DECODE(A);
 jmp010: if (IS_FALSE(A)) { pc += OBC(code); } STEP; // jump on false(A)

 jmp111: DECODE(A);
 jmp011: if (IS_FALSE(A)) { STEP; } // jump on true(A)
 jmp000: pc += OBC(code); STEP;     // unconditional

 jmp100: DECODE(A);
 jmp101: if (!IS_FALSE(A)) { STEP; }
 jmp001: pc -= OBC(code); STEP;

    LABEL(func):
    func:
    assert(IS_PROTO(A));
    *ptrC = VAL_OBJ(Func::alloc((Proto *) A, ups, regs));
    STEP;

    LABEL(get): B=regs[OB(code)]; get: *ptrC = doGet(A, B); STEP;
    LABEL(set): B=regs[OB(code)]; set: doSet(*ptrC, A, B);  STEP;

    LABEL(retur): retur: {
        regs[0] = A;
        if (retInfo.size == 0) { return A; }
        RetInfo *ri = retInfo.top();
        pc   = ri->pc;
        regs = ri->regs;
        ups  = ri->ups;
        retInfo.pop();
        STEP;
    }

    LABEL(call): B=regs[OB(code)]; call: { 
        assert(IS_INT(A));
        ERR(TAG(B) != T_OBJ || B == NIL, E_CALL_NIL);

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

    LABEL(move): move: *ptrC = A; STEP;
    LABEL(add):  B=regs[OB(code)]; add: *ptrC = IS_INT(A) && IS_INT(B) ? VAL_INT(getInteger(A) + getInteger(B)) : doAdd(A, B); STEP;     
    LABEL(sub):  B=regs[OB(code)]; sub: *ptrC = doSub(A, B); STEP;
    LABEL(mul):  B=regs[OB(code)]; mul: *ptrC = doMul(A, B); STEP;
    LABEL(div):  B=regs[OB(code)]; div: *ptrC = doDiv(A, B); STEP;
    LABEL(mod):  B=regs[OB(code)]; mod: *ptrC = doMod(A, B); STEP;
    LABEL(pow):  B=regs[OB(code)]; pow: *ptrC = doPow(A, B); STEP;

#define BITOP(op, A, B) IS_INT(A) && IS_INT(B) ? VAL_INT(getInteger(A) op getInteger(B)) : ERROR(E_WRONG_TYPE)
    LABEL(andb): B=regs[OB(code)]; andb: *ptrC = BITOP(&,  A, B); STEP;
    LABEL(orb):  B=regs[OB(code)]; orb:  *ptrC = BITOP(|,  A, B); STEP;
    LABEL(xorb): B=regs[OB(code)]; xorb: *ptrC = BITOP(^,  A, B); STEP;

    LABEL(shl):  B=regs[OB(code)]; shl: *ptrC = BITOP(<<, A, B); STEP;
    LABEL(shr):  B=regs[OB(code)]; shr: *ptrC = BITOP(>>, A, B); STEP;

    LABEL(notb): notb: *ptrC = IS_INT(A)? VAL_INT(~getInteger(A)):ERROR(E_WRONG_TYPE); STEP;
    LABEL(notl): notl: *ptrC = IS_FALSE(A) ?  TRUE  : FALSE; STEP;
    LABEL(eq):   B=regs[OB(code)]; eq:  *ptrC = equals(A, B)  ? TRUE : FALSE; STEP;
    LABEL(neq):  B=regs[OB(code)]; neq: *ptrC = !equals(A, B) ? TRUE : FALSE; STEP;

    LABEL(lt):   B=regs[OB(code)]; lt: *ptrC = lessThan(A, B) ? TRUE : FALSE; STEP;
    LABEL(le):   B=regs[OB(code)]; le: *ptrC = A==B || equals(A, B) || lessThan(A, B) ? TRUE : FALSE; STEP;

    LABEL(len):  len: *ptrC = VAL_INT(len(A)); STEP;

    LABEL2(call);
    LABEL2(retur);
    LABEL2(func);
    LABEL2(get);
    LABEL2(set);
    LABEL2(move);
    LABEL2(len);
    LABEL2(add);
    LABEL2(sub);
    LABEL2(mul);
    LABEL2(div);
    LABEL2(mod);
    LABEL2(pow);
    LABEL2(andb);
    LABEL2(orb);
    LABEL2(xorb);
    LABEL2(notb);
    LABEL2(shl);
    LABEL2(shr);
    LABEL2(notl);
    LABEL2(eq);
    LABEL2(neq);
    LABEL2(lt);
    LABEL2(le);
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
