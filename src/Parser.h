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
    void emitCode(unsigned code);
    int  emitHole();
    void emitPatch(unsigned pos, unsigned code);
    void emitJumpFalse(unsigned pos, Value cond);
    void emitJumpTrue(unsigned pos, Value cond);
    void patchOrEmitMove(int dest, Value a);

    Value codeUnary(int top, int op, Value a);
    Value codeBinary(int top, int op, Value a, Value b);

    void advance();
    void consume(int token);

    Value expr(int top);
    Value subExpr(int top, int limit);
    Value suffixedExpr(int top);
    Value arrayExpr(int top);
    Value mapExpr(int top);
    Value funcExpr(int top);

    void parList();
    void block();
    void statList();
    void statement();

    void varStat();
    void ifStat();
    void assignStat();
    void exprOrAssignStat();

public:
    static Func *parseFunc(const char *text);
    static Func *parseStatList(const char *text);

    static int parseStatList(Proto *proto, SymbolTable *symbols, const char *text);
    static void close(Proto *proto);

    void defineName(const char *name, Value a);
};
