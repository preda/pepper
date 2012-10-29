#include "VM.h"
#include "Array.h"
#include "Map.h"
#include "Func.h"

typedef unsigned char byte;

#define OP(c)  (c & 0x1f)
#define OB(c) ((c >>  5) & 0x1ff)
#define OC(c) ((c >> 14) & 0x1ff)
#define OD(c) ((c >>  5) & 0x3ffff)
#define OA(c)  (c >> 23)

#define STEP code=*pc++; ra=regs+OA(code); b=OB(code); c=OC(code); B=regs[b]; C=regs[c]; goto *dispatch[OP(code)]

#define INT(x) VAL_INT((signed char)x)
#define BINOP(op) op: b=regs[OB(code)]; tagB=TAG(b); c=regs[OC(code)]; tagC=TAG(c);

#define ERROR(mes) { error=mes; goto end; }

#define DECODE(v, B) {int inc=0; v=decodeInlineValue(B, pc, &inc); pc+=inc;}

#define IF_NUMBER_BINOP(op) if (IS_NUMBER_TAG(ta) && IS_NUMBER_TAG(tb)) { *d = ta==INTEGER && tb==INTEGER ? VAL_INT(getInteger(a) op getInteger(b)) : VAL_DOUBLE(getDouble(a) op getDouble(b)); }

static int LEN(Value v) {
    if (v && TAG(v) == OBJECT) {
        switch (((Object *)v)->type) {
        case ARRAY:  return ((Array *) v)->vect.size;
        case MAP:    return ((Map *)   v)->size;
        case STRING: return ((String *)v)->size;
        }
    }
    return 0;
}

struct RetInfo {
    unsigned *pc;
    Value *regs;
};

static Value arrayAdd(Value a, Value b) {
    int len = LEN(a) + LEN(b);
    if (len == 0) {
        int tb = TAG(b);
        return (tb == ARRAY || tb == MAP || tb == STRING) ? ARRAY0 : ERR;
    }
    Array *array = Array::alloc(len);
    return (array->appendArray(a) && array->appendArray(b)) ? VALUE(OBJECT, array) : ERR;
}

static Value mapAdd(Value a, Value b) {
    int len = LEN(a) + LEN(b);
    if (len == 0) {
        int tb = TAG(b);
        return (tb == ARRAY || tb == MAP || tb == STRING) ? MAP0 : ERR;
    }
    Map *map = ((Map *) a)->copy();
    return map->appendArray(b) ? VALUE(OBJECT, map) : ERR;
}

Value doAdd(Value a, Value b) {
    return 
        IS_INTEGER(a) && IS_INTEGER(b) ? VAL_INT(getInteger(a) + getInteger(b))  :
        IS_NUMBER(a)  && IS_NUMBER(b)  ? VAL_DOUBLE(getDouble(a) + getDouble(b)) :
        IS_STRING(a) ? stringConcat(a, b) :
        IS_ARRAY(a)  ? arrayAdd(a, b) :
        IS_MAP(a)    ? mapAdd(a, b) :
        ERR;
}

Value doSub(Value a, Value b) {
    return 
        IS_INTEGER(a) && IS_INTEGER(b) ? VAL_INT(getInteger(a) + getInteger(b))  :
        IS_NUMBER(a)  && IS_NUMBER(b)  ? VAL_DOUBLE(getDouble(a) + getDouble(b)) :
        ERR;
}

Value doMul(Value a, Value b) {
    return 
        IS_INTEGER(a) && IS_INTEGER(b) ? VAL_INT(getInteger(a) * getInteger(b))  :
        IS_NUMBER(a)  && IS_NUMBER(b)  ? VAL_DOUBLE(getDouble(a) * getDouble(b)) :
        ERR;
}

Value doDiv(Value a, Value b) {
    if (IS_INTEGER(a) && IS_INTEGER(b)) {
        long long vc = getInteger(b);
        return vc == 0 ? ERR : VAL_INT(getInteger(a) / vc);
    } else if (IS_NUMBER(a) && IS_NUMBER(b)) {
        double vc = getDouble(b);
        return vc == 0 ? ERR : VAL_DOUBLE(getDouble(a) / vc);
    } else {
        return ERR;
    }
}

Value doMod(Value a, Value b) {
    return NIL;
}

Value doPow(Value a, Value b) {
    return NIL;
}

Value doAnd(Value a, Value b) {
    return IS_INTEGER(a) && IS_INTEGER(b) ? VAL_INT(getInteger(a) & getInteger(b)) : ERR;
}

Value doOr(Value a, Value b) {
    return IS_INTEGER(a) && IS_INTEGER(b) ? VAL_INT(getInteger(a) | getInteger(b)) : ERR;
}

Value doXor(Value a, Value b) {
    return IS_INTEGER(a) && IS_INTEGER(b) ? VAL_INT(getInteger(a) ^ getInteger(b)) : ERR;
}

int VM::run(unsigned *pc) {
    void *dispatch[32] = {
        &&call, &&return_, &&move,
        &&add, &&sub, &&mul, &&div, &&mod, &&pow, &&and_, &&or_, &&xor_,
        &&end};

    int stackSize = 1024;
    Value *stack = (Value *) calloc(stackSize, sizeof(Value));
    Value *regs  = stack;
    unsigned code;
    Value *ra;
    int b, c;
    Value B, C;
    Vector<RetInfo> retInfo;
    STEP;

 call: {
    // A: function to call
    // b: start of args
    // c: n args    
        Value A = *ra;
        if (TAG(A) != OBJECT) { return 1; }
        Func *f = (Func *) A;
        Value *base = regs + b;
        const int effArgs = c;
        if (f->type == FUNC) {
            Proto *proto = f->proto;
            int nArgs = f->proto->nArgs;
            for (Value *p=base+effArgs, *end=base+nArgs; p < end; ++p) {
                *p = NIL;
            }
            if (proto->hasEllipsis) {
                if (effArgs < nArgs) {
                    base[nArgs-1] = ARRAY0; // VALUE(ARRAY, 0);
                } else {
                    int nExtra = effArgs - nArgs;
                    Array *a = Array::alloc(nExtra);
                    for (Value *p=base+nArgs-1, *end=base+effArgs; p < end; ++p) {
                        a->push(*p);
                    }
                    base[nArgs-1] = VAL_OBJ(a);
                }
            }
            if (*pc == RETURN) {
                memmove(regs, base, nArgs * sizeof(Value));
                base = regs;
            } else {
                RetInfo *ret = retInfo.push();
                ret->pc   = pc;
                ret->regs = regs;
            }
            pc = proto->pc;
        } else if (f->type == CFUNC) {
            CFunc *cf = (CFunc *) A;
            cf->call(base, effArgs);
        } else { return 2; }
        STEP;
    }

 return_:
    RetInfo *ri = retInfo.top();
    regs = ri->regs;
    pc   = ri->pc;
    retInfo.pop();
    STEP;

 move:
    switch(b) {
    case 0: *ra = regs[c];               STEP;
    case 1: *ra = ((short)(c<<7))>>7;    STEP;
    case 2: *ra = VALUE(c, 0);           STEP;
    case 3: *ra = *(Value *)pc; pc += 2; STEP;
    case 4: *ra = upvals[c];             STEP;
    case 5: upvals[c] = *ra;             STEP;        
    }
    return 2;
     
 add: *ra = doAdd(B, C); STEP;     
 sub: *ra = doSub(B, C); STEP;
 mul: *ra = doMul(B, C); STEP;
 div: *ra = doDiv(B, C); STEP;
 mod: *ra = doMod(B, C); STEP;
 pow: *ra = doPow(B, C); STEP;
 and_: *ra = NIL;        STEP;
 or_:  *ra = NIL;        STEP;
 xor_: *ra = NIL;        STEP;
 end: return;
}

/*
static Value decodeInlineValue(int type, unsigned *p, int *incPC) {
    switch (type) {
    case 0: 
        *incPC = 2;
    }
    unsigned size = *p++;
    if (type == 1) { // STRING of chars
        String *string = String::alloc((char *)p, size);
        *incPC = (size + 3) / 4 + 1;
        return VAL_OBJ(string);
    } else if (type == 2) { // ARRAY of Values
        Array *array = Array::alloc(size);
        for (int i = 0; i < size; ++i) {
            array->push(*(Value *)p);
            p += 2;
        }
        *incPC = size * 2 + 1;
        return VAL_OBJ(array);
    } else {
        // error
        *incPC = 1;
        return NIL;
    }
}
*/
