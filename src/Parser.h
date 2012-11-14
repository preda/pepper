#pragma once

#include "Value.h"

class Lexer;
class SymbolTable;
class Proto;
class Func;
struct SymbolData;

enum {
    E_VAR_NAME = 1,
    E_NAME_NOT_FOUND,
    E_TODO,
    E_EXPECTED = 256,
    
};

class Parser {
    Proto *proto;
    SymbolTable *syms;
    Lexer *lexer;
    
    Value codeUnary(int op, Value a);
    SymbolData createUpval(Proto *proto, u64 name, SymbolData sym);
    SymbolData lookupName(u64 name);

    Value maybeAllocConst(Value a);
    Value genCode(int op, int dest, Value a, Value b = VAL_REG(0));
    Value genCode(int op, Value a, Value b = VAL_REG(0));
    Value codeBinary(int op, Value a, Value b);
    void error(int err) __attribute__ ((noreturn));

    Parser(Proto *proto, SymbolTable *syms, Lexer *lexer);
    ~Parser();

public:
    static Func *parseFunc(const char *text);
    static void parseStatList(Proto *proto, SymbolTable *symbols, const char *text);


    void advanceToken();
    void consume(int t);

    void var();
    
    Value expr();
    Value subExpr(int limit);
    Value simpleExpr();
    Value suffixedExpr();
    Value primaryExpr();

    void block();
    void statList();
    void statement();
    void enterBlock();
    void leaveBlock();
};
