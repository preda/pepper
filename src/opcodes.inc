_(JMP),    //00 pc += D
_(JT),     //01 if C,  pc += D
_(JF),     //02 if !C, pc += D
_(FOR),    //03 set up for loop: C=[C+2]; if C>=[C+1],  pc += D
_(LOOP),   //04 loop for loop:   inc(C);  if C!==[C+1], pc += D
_(JLT),    //05 if A < B, pc += C
_(JNIS),   //06 if !(A is B) pc += C
_(CALL),   //07 C is base, A is func, B is nArgs
_(RET),    //08 return A
_(FUNC),   //09 C = made func, A is proto
_(GETI),   //0A index get, C=A[B]
_(SETI),   //0B index set, C[A]=B
_(GETF),   //0C field get, C=A.B          
_(SETF),   //0D field set, C.A=B
_(GETS),   //0E slice get C=A[B:B+1]
_(SETS),   //0F slice set C[A:A+1]=B
_(MOVEUP), //10 C dest, A is src
_(MOVE_R), //11 C = A
_(MOVE_I), //12 C = integer(D)
_(MOVE_C), //13 C = inline value
_(MOVE_V), //14 C = special const

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
