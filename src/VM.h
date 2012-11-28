#pragma once

#include "Value.h"
#include "Vector.h"
#include "RetInfo.h"
#include "Func.h"

Value doAdd(Value a, Value b);
Value doSub(Value a, Value b);
Value doMul(Value a, Value b);
Value doDiv(Value a, Value b);

Value doMod(Value a, Value b);
Value doPow(Value a, Value b);

union CodeBytes {
    unsigned code;
    struct {
        byte op;
        byte c;
        byte a;
        byte b;
    };
    struct {
        short dummy;
        short d;
    };
};

inline int OP(unsigned code) { return CodeBytes{code:code}.op; }
inline int OC(unsigned code) { return CodeBytes{code:code}.c; }
inline int OA(unsigned code) { return CodeBytes{code:code}.a; }
inline int OB(unsigned code) { return CodeBytes{code:code}.b; }
inline int OD(unsigned code) { return CodeBytes{code:code}.d; }

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

class VM {
    Value *stack;
    unsigned stackSize;
    Func *activeFunc;

    Vector<RetInfo> retInfo;

    Value *maybeGrowStack(Value *regs);

 public:
    VM();
    ~VM();
    Value run(Func *f);
};
