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

class VM {
 public:
    const char *error;

    enum {
        CALL, RETURN, MOVE,

        // Binary
        ADD, SUB, MUL, DIV, MOD, POW, AND, OR, XOR,

        // Unary
        NOT, LEN,

        END,
    };
    
    int run(unsigned *code);
};
