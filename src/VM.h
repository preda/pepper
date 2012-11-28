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

#define OP(c) ((byte)  (c))
#define OC(c) ((byte)  (c >> 8))
#define OA(c) ((byte)  (c >> 16))
#define OB(c) ((byte)  (c >> 24))
#define OD(c) (c >> 16)

#define CODE_CAB(op, c, a, b) ((op) | ((byte)(c) << 8) | ((byte)(a) << 16) | ((byte)(b) << 24))

#define CODE_CD(op, c, d) ((op) | ((byte)(c) << 8) | ((d) << 16))

enum {
    JMP,    // D is offest
    JMPF,   // C is cond, D offset
    JMPT,   // C is cond, D offset
    CALL,   // C is base, A is func, B is nArgs
    RET,    // return A
    FUNC,   // C = made func, A is proto
    GET,    // C=A[B]
    SET,    // C[A]=B
    MOVE,   // hi-level for any move
    MOVEUP = MOVE, // C dest, A is src
    MOVE_R,        // C dest, A is src
    MOVE_I,        // D is int src
    MOVE_C,
    LEN,
    NOTL,
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    POW,    
    AND,
    OR,
    XOR,
    SHL,
    SHL_RR = SHL,
    SHL_RI,
    SHR,
    SHR_RR = SHR,
    SHR_RI,
    EQ,
    NEQ,
    LT,
    LE,
    N_OPCODES // number of opcodes
};

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
