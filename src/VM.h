// Copyright (C) 2012 - 2013 Mihai Preda
#pragma once

#include "Value.h"
#include "Func.h"

#define N_CONST_UPS (7)

enum {
    CONST_NIL = 0,
    CONST_EMPTY_STRING = 1,
    CONST_EMPTY_ARRAY  = 2,
    CONST_EMPTY_MAP    = 3,
};

Value doAdd(GC *gc, Value a, Value b);
Value doMod(Value a, Value b);
Value doPow(Value a, Value b);

#define BITOP(op, A, B) (IS_NUM(A) && IS_NUM(B) ? VAL_NUM((unsigned)GET_NUM(A) op (unsigned)GET_NUM(B)) : ERROR(E_WRONG_TYPE))

#define BINOP(op, a, b) (IS_NUM(a) && IS_NUM(b) ? VAL_NUM(GET_NUM(a) op GET_NUM(b)) : ERROR(E_WRONG_TYPE))

union CodeBytes {
    unsigned code;
    struct {
        byte op;
        byte c;
        byte a;
        byte b;
    };
    struct {
        byte dummy1;
        signed char sc;
        signed char sa;
        signed char sb;
    };
    struct {
        short dummy2;
        short d;
    };
};

inline int OP(unsigned code) { return CodeBytes{code:code}.op; }
inline int OC(unsigned code) { return CodeBytes{code:code}.c; }
inline int OA(unsigned code) { return CodeBytes{code:code}.a; }
inline int OB(unsigned code) { return CodeBytes{code:code}.b; }
inline int OD(unsigned code) { return CodeBytes{code:code}.d; }
inline int OSC(unsigned code) { return CodeBytes{code:code}.sc; }
inline int OSB(unsigned code) { return CodeBytes{code:code}.sb; }

#define CODE_CAB(op, c, a, b) ((op) | ((byte)(c) << 8) | ((byte)(a) << 16) | ((byte)(b) << 24))

#define CODE_CD(op, c, d) ((op) | ((byte)(c) << 8) | ((d) << 16))

#define _(n) n
enum {
#include "opcodes.inc"
    N_OPCODES, // number of opcodes
    MOVE = MOVEUP, // hi-level for any move
    SHL  = SHL_RR,
    SHR  = SHR_RR,
};
#undef _

bool opcodeHasDest(int opcode);

class Stack;
class Pepper;
class Types;
class Map;
class Array;

class VM {
 public:
    GC *gc;
    Types *types;
    
 private:
    Pepper *_pepper;
    const Value constUps[N_CONST_UPS];
    Array *emptyArray;
    Map *emptyMap;

    void copyUpvals(Func *f, Value *regs);
    Value *maybeGrowStack(Value *regs);
    Value getField(Value a, Value b);
    int call(Value fval, int nArg, Value *base, Stack *stack); // returns non-zero on error
    
 public:
    VM(Pepper *pepper);
    ~VM();

    Value run(Value fv, int nArg, Value *args);
    Pepper *pepper() { return _pepper; }
};
