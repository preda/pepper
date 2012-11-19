#pragma once

#include "Value.h"

Value doAdd(Value a, Value b);
Value doSub(Value a, Value b);
Value doMul(Value a, Value b);
Value doDiv(Value a, Value b);

Value doMod(Value a, Value b);
Value doPow(Value a, Value b);


Value doAnd(Value a, Value b);
Value doOr(Value a, Value b);
Value doXor(Value a, Value b);

#define OP(c) ((byte)  (c))
#define OA(c) ((byte)  (c >> 8))
#define OB(c) ((byte)  (c >> 16))
#define OC(c) ((byte)  (c >> 24))
// #define OD(c) ((short) (c >> 16))

enum {
    JMP,     // pc += int(A) if not B
    CALL,    // base=C, nArgs = int(A), func(B)
    RET,     // return A;
    CLOSURE, // C = closure(proto(A))
    
    MOVE,

    // table e.g. a[i]
    GET, SET,
    
    // Binary
    ADD, SUB, MUL, DIV, MOD, POW, AND, OR, XOR,
    
    // Unary
    NOT, LEN,
};

bool opcodeHasDest(int opcode);

int vmrun(unsigned *pc);

class VM {
 public:
    const char *error;
    
    int run(unsigned *code);
};
