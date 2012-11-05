#pragma once

#include "Lexer.h"
#include "SymbolTable.h"
#include "Vector.h"
#include "Value.h"

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
    Vector<unsigned> bytecode;

    Value codeUnary(int op, Value a);
    SymbolData createUpval(u64 name, SymbolData sym, int level);
    SymbolData lookupName(u64 name);

public:
    Parser(Lexer *lexer);

    void advanceToken();
    void consume(int t);

    void var();
    
    Value expr(int dest);
    Value subExpr(int dest, int limit);
    Value simpleExpr();
    Value suffixedExpr();

    void block();
    void statList();
    void statement();
    void simpleExp();
    int name();
    void enterBlock();
    void leaveBlock();
};
