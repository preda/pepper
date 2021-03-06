#pragma once

#include "Value.h"

class Lexer;
class SymbolTable;
class Proto;
class Func;
class GC;
class Pepper;

class Parser {
    Proto *proto;
    SymbolTable *syms;
    Lexer *lexer;
    GC *gc;

    Parser(GC *gc, Proto *proto, SymbolTable *syms, Lexer *lexer);
    ~Parser();
    
    int createUpval(Proto *proto, int protoLevel, Value name, int level, int slot);
    int lookupSlot(Value name);

    int  emitHole();
    void emit(unsigned top, int op, int dest, Value a, Value b);
    void emitJump(int pos, int op, Value a, int to);
    void emitCode(unsigned top, int op, int dest, Value a, Value b);
    void patchOrEmitMove(int top, int dest, Value a);

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
    Value callExpr(int top, Value func, Value self);

    void parList();
    
    // returns true if last statement was "return".
    bool block();
    bool insideBlock();
    bool lambdaBody();

    bool statList();

    // returns true if statement was "return".
    bool statement();

    void ifStat();
    void whileStat();
    void forStat();
    Proto *parseProto(int *outSlot);
    Value mapSpecialConsts(Value a);
    static Func  *makeFunc(GC *gc, Proto *proto, Value *upsTop, int recSlot);

public:
    static Func *parseFunc(GC *gc, SymbolTable *syms, Value *upsTop, const char *text);
    static Func *parseStatList(GC *gc, SymbolTable *syms, Value *upsTop, const char *text);
    static int parseStatList(GC *gc, Proto *proto, SymbolTable *symbols, const char *text);
    static Func *parseInEnv(Pepper *pepper, const char *text, bool isFunc);
};
