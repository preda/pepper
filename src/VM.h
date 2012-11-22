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
#define OA(c) ((byte)  (c >> 8))
#define OB(c) ((byte)  (c >> 16))
#define OC(c) ((byte)  (c >> 24))
// #define OD(c) ((short) (c >> 16))

enum {
    JMP,      // pc += int(A) if not B
    CALL,     // base=C, nArgs = int(A), func(B)
    RET,      // return A;
    FUNC,     // C = func(proto(A))

    GET, // C=A[B]
    SET, // C[A]=B
    MOVE,
    LEN,

    ADD, SUB,
    MUL, DIV, 
    MOD, POW,

    AND, OR,
    XOR, NOTB,
    SHL, SHR,

    NOTL, 
    EQ, NEQ,
    LT, LE, // numeric comparison ==, <, <=    
};

bool opcodeHasDest(int opcode);

class VM {
    Value *stack;
    unsigned stackSize;
    Vector<RetInfo> retInfo;

    Value *maybeGrowStack(Value *regs);

 public:
    VM();
    ~VM();
    Value run(Func *f);
};
