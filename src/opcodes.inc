_(JMP),    // pc += D
_(JT),     // if C,  pc += D
_(JF),     // if !C, pc += D
_(FOR),    // set up for loop: C=[C+2]; if C>=[C+1],  pc += D
_(LOOP),   // loop for loop:   inc(C);  if C!==[C+1], pc += D
_(JLT),    // if A < B, pc += C
_(JNIS),   // if !(A is B) pc += C
_(CALL),   // C is base, A is func, B is nArgs
_(RET),    // return A
_(FUNC),   // C = made func, A is proto
_(GETI),   // index get, C=A[B]
_(SETI),   // index set, C[A]=B
_(GETF),   // field get, C=A.B          
_(SETF),   // field set, C.A=B
_(GETS),   // slice get C=A[B:B+1]
_(SETS),   // slice set C[A:A+1]=B
_(MOVEUP), // C dest, A is src
_(MOVE_R), // C dest, A is src
_(MOVE_I), // D is int src
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
_(IS),
_(NIS),
_(LT),
_(LE),
