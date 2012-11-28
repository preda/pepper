    _(JMP),    // D is offest
    _(JMPF),   // C is cond, D offset
    _(JMPT),   // C is cond, D offset
    _(CALL),   // C is base, A is func, B is nArgs
    _(RET),    // return A
    _(FUNC),   // C = made func, A is proto
    _(GET),    // C=A[B]
    _(SET),    // C[A]=B
    _(MOVEUP), // C dest, A is src
    _(MOVE_R),        // C dest, A is src
    _(MOVE_I),        // D is int src
    _(MOVE_C),
    _(LEN),
    _(NOTL),
    _(ADD),
    _(SUB),
    _(MUL),
    _(DIV),
    _(MOD),
    _(POW),    
    _(AND),
    _(OR),
    _(XOR),
    _(SHL_RR),
    _(SHL_RI),
    _(SHR_RR),
    _(SHR_RI),
    _(EQ),
    _(NEQ),
    _(LT),
    _(LE),
