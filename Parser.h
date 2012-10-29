#pragma once

#include "Lexer.h"
#include "SymbolTable.h"

struct Function {
    
};

class Block {
 public:
    int nVars;
    int regTop;
};

class Parser {
    SymbolTable symbolTable;
    int top;

    Lexer *lexer;
    int token;
    TokenInfo tokenInfo;

    /*
    void addFun(const char *name, ) {
        symbolTable.set(name, top);        
    }
    */

public:
    Parser(Lexer *lexer);

    void advanceToken();
    void consume(int t);

    void var();
    void expr(int destReg);

    void block();
    void statList();
    void statement();
    void simpleExp();
    int name();
    void enterBlock();
    void leaveBlock();
};
