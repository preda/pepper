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

enum {
    CALL, RETURN, MOVE,
    
    // Binary
    ADD, SUB, MUL, DIV, MOD, POW, AND, OR, XOR,
    
    // Unary
    NOT, LEN,
    
    END,
};

#define OP(c) ((byte) (c))
#define OC(c) ((byte) (c >> 8))
#define OA(c) ((byte) (c >> 16))
#define OB(c) ((byte) (c >> 24))
#define OAB(c) ((unsigned short) (c >> 16))

int vmrun(unsigned *pc);

void bytecodePrint(unsigned *bytecode, int size);

class VM {
 public:
    const char *error;
    
    int run(unsigned *code);
};
