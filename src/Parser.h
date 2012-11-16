#pragma once

#include "Value.h"

class Lexer;
class SymbolTable;
class Proto;
class Func;
struct SymbolData;

class Parser {
    Proto *proto;
    SymbolTable *syms;
    Lexer *lexer;

    Parser(Proto *proto, SymbolTable *syms, Lexer *lexer);
    ~Parser();
    
    SymbolData createUpval(Proto *proto, u64 name, SymbolData sym);
    SymbolData lookupName(u64 name);
    int lookupSlot(u64 name);

    Value maybeAllocConst(Value a);
    byte getRegValue(Value a);

    unsigned makeCode(int op, Value c, Value a, Value b);
    // unsigned makeCode(int op, Value a, int offset);
    void emitCode(unsigned code);
    int  emitHole();
    void emitPatch(unsigned pos, unsigned code);
    void emitPatchJumpHere(unsigned pos, Value cond = FALSE);
    void patchOrEmitMove(int dest, Value a);

    Value codeUnary(int op, Value a);
    Value codeBinary(int op, Value a, Value b);

    void advance();

public:
    static Func *parseFunc(const char *text);
    static void parseStatList(Proto *proto, SymbolTable *symbols, const char *text);

    void consume(int t);
    
    Value expr();
    Value subExpr(int limit);
    Value simpleExpr();
    Value suffixedExpr();
    Value primaryExpr();

    void block();
    void statList();
    void statement();

    void varStat();
    void ifStat();
    void assignStat();
    void exprOrAssignStat();

    void enterBlock();
    void leaveBlock();
};
