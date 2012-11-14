#include "Parser.h"
#include "Lexer.h"
#include "VM.h"
#include "Object.h"
#include "Array.h"
#include "Map.h"
#include "String.h"
#include "Func.h"
#include "Proto.h"
#include "SymbolTable.h"

#include <assert.h>

#define UNUSED VAL_REG(0)
#define TRUE  VAL_INT(1)
#define FALSE VAL_INT(0)

void Parser::error(int err) {
    fprintf(stderr, "parser error %d\n", err);
    assert(0);
}

Func *Parser::parseFunc(const char *text) {
    Proto proto;
    SymbolTable syms;
    Lexer lexer(text);
    // Parser(&proto, &syms, &lexer).func();
    // TODO
    return 0;
}

void Parser::parseStatList(Proto *proto, SymbolTable *syms, const char *text) {
    Lexer lexer(text);
    Parser(proto, syms, &lexer).statList();
}

Parser::Parser(Proto *proto, SymbolTable *syms, Lexer *lexer) {
    this->proto = proto;
    this->syms  = syms;
    this->lexer = lexer;
    lexer->advance();
}

Parser::~Parser() {
}

void Parser::advanceToken() {
    lexer->advance();
}

void Parser::consume(int t) {
    if (t == lexer->token) {
        lexer->advance();
    } else {
        error(E_EXPECTED + t);
    }
}

void Parser::block() {
    consume('{');
    enterBlock();
    statList();
    leaveBlock();
    consume('}');
}

void Parser::enterBlock() {
}

void Parser::leaveBlock() {
}

void Parser::statList() {
    while (lexer->token != '}' && lexer->token != TK_END) {
        statement();
    }
}

void Parser::statement() {
    switch (lexer->token) {
    case TK_VAR: var(); break;
        /*
    case TK_IF: ifstat(); break;
    case TK_FOR: forstat(); break;
    default: exprstat(); break;
        */
    }
}

void Parser::var() {
    consume(TK_VAR);
    if (lexer->token == TK_NAME) {
        u64 name = lexer->info.nameHash;
        consume(TK_NAME);
        consume('=');
        Value a = expr(); // sympleExpr() ?
        int reg = proto->top++;
        syms->set(name, reg+1);
        genCode(MOVE, reg, a);
    } else {
        error(E_VAR_NAME);
    }
}

static bool isUnaryOp(int token) {
    return token=='!' || token=='-' || token=='#' || token=='~';
}

Value Parser::maybeAllocConst(Value a) {
    int ta = TAG(a);
    if (ta==REGISTER || ta==ARRAY || ta==MAP || ta==STRING ||
        (ta==INTEGER && getInteger(a)>=-64 && getInteger(a)<64)) {
        return a;
    }
    proto->ups.push(0);
    proto->consts.push(a);
    return VAL_REG(- proto->ups.size);
}

static byte getRegValue(Value a) {
    const int ta = TAG(a);
    return
        ta==REGISTER && (int)a >= 0 ? (int) a :
        ta==REGISTER ? 0x80 | (-(int)a-1) :
        ta==ARRAY || ta==MAP || ta==STRING ? (0xf8 | ta) : ((unsigned)a & 0x7f);
}

static inline unsigned PACK4(unsigned op, unsigned dest, unsigned a, unsigned b) {
    return op | (dest<<8) | (a<<16) | (b<<24);
}

Value Parser::genCode(int op, Value a, Value b) {
    return genCode(op, proto->top++, a, b);
}

Value Parser::genCode(int op, int dest, Value a, Value b) {
    // int dest = top++;
    a = maybeAllocConst(a);
    b = maybeAllocConst(b);
    bool aIsReg = IS_REGISTER(a) && (int)a >= 0;
    byte ra = getRegValue(a);
    
    bool bIsReg = IS_REGISTER(b) && (int)b >= 0;
    byte rb = getRegValue(b);

    proto->code.push(PACK4(op | (aIsReg?0:0x20) | (bIsReg?0:0x40), dest, ra, rb));
    return VAL_REG(dest);
}

Value Parser::codeUnary(int op, Value a) {
    switch (op) {
    case '!': return 
        a==NIL ? TRUE :
        IS_NUMBER(a) ? (getDouble(a)==0 ? TRUE : FALSE) :
        IS_REGISTER(a) ? genCode(NOT, a) : FALSE;
             
    case '-': return
        IS_INTEGER(a) ? VAL_INT(-getInteger(a)) :
        IS_DOUBLE(a)  ? VAL_DOUBLE(-getDouble(a)) :
        IS_REGISTER(a) ? genCode(SUB, VAL_INT(0), a) : ERR;

    case '~': return
        IS_INTEGER(a) ? VAL_INT(~getInteger(a)) :
        IS_REGISTER(a) ? genCode(XOR, a, VAL_INT(-1)) : ERR;
        
    case '#': return
        IS_ARRAY(a) || IS_STRING(a) || IS_MAP(a) ? VAL_INT(len(a)) :
        IS_REGISTER(a) ? genCode(LEN, a) : ERR;
    }
    assert(false);
}

static Value foldBinary(int op, Value a, Value b) {
    if (!IS_REGISTER(a) && !IS_REGISTER(b)) {
        switch (op) {
        case '+': return doAdd(a, b);
        case '-': return doSub(a, b);
        case '*': return doMul(a, b);
        case '/': return doDiv(a, b);
        case '%': return doMod(a, b);
        case '^': return doPow(a, b);            
        }
    }
    return NIL;
}

Value Parser::codeBinary(int op, Value a, Value b) {
    Value c = foldBinary(op, a, b);
    if (c != NIL) { return c; }
    int opcode = 
        op == '+' ? ADD :
        op == '-' ? SUB :
        op == '*' ? MUL :
        op == '/' ? DIV :
        op == '%' ? MOD :
        op == '^' ? POW : -1;
    assert(op >= 0);
    return genCode(opcode, a, b);
}

static int binaryPriorityLeft(int token) {
    switch (token) {
    case '+': case '-': return 6;
    case '*': case '/': case '%': return 7;
    case '^': return 10;
    case '='+TK_EQUAL: case '<': case '<'+TK_EQUAL: case '!'+TK_EQUAL:
    case '>': case '>'+TK_EQUAL: return 3;
    case '&': return 2;
    case '|': return 1;
    default : return -1;
    }
}

static int binaryPriorityRight(int token) {
    int left = binaryPriorityLeft(token);
    return token == '^' ? left-1 : left;
}

SymbolData Parser::createUpval(Proto *proto, u64 name, SymbolData sym) {
    if (sym.level < proto->level - 1) {
        sym = createUpval(proto->up, name, sym);
    }
    assert(sym.level == proto->level - 1);
    proto->ups.push(sym.slot);
    return syms->set(proto->level, name, -proto->ups.size);
}

SymbolData Parser::lookupName(u64 name) {
    SymbolData s = syms->get(name);
    if (s.kind != KIND_EMPTY) {
        assert(s.level <= proto->level);
        if (s.level < proto->level) {
            s = createUpval(proto, name, s);
        }
    }
    return s;
}

Value Parser::primaryExpr() {
    switch (lexer->token) {
    case '(': {
        advanceToken();
        Value a = expr();
        consume(')');
        return a;
    }
        
    case TK_NAME: {
        u64 name = lexer->info.nameHash;
        consume(TK_NAME);
        SymbolData sym = syms->get(name);
        if (sym.kind == KIND_EMPTY) {
            error(E_NAME_NOT_FOUND);
        }
        
        if (sym.level == proto->level) {
            return VAL_REG(sym.slot);
        } else {
            // create new upval in current level
            createUpval(proto, name, sym);
        }
    }
    }
    error(E_TODO);
}

Value Parser::suffixedExpr() {
    //TODO
    return 0;
    /*
    Value a = primaryExpr();
    while (true) {
        switch (lexer->token) {
        case '(':
            funcargs();
            break;

        default:
            
        }
    }
    */
}

Value Parser::simpleExpr() {
    printf("enter simpleExpr\n");
    switch (lexer->token) {
    case TK_INTEGER: return VAL_INT(lexer->info.intVal);
    case TK_DOUBLE:  return VAL_DOUBLE(lexer->info.doubleVal);
    case TK_STRING:  return VAL_STRING(lexer->info.strVal, lexer->info.strLen);
    case TK_NIL:     return NIL;

    case '[': // array TODO
        break;

    case '{': // map TODO
        break;

    case TK_FUNC:
        break;

    default:
        return suffixedExpr();
    }
    error(E_TODO);
}

Value Parser::subExpr(int limit) {
    printf("enter subExpr %d\n", limit);
    Value a;
    if (isUnaryOp(lexer->token)) {
        int op = lexer->token;
        advanceToken();
        a = codeUnary(op, subExpr(8));
    } else {
        a = simpleExpr();
        advanceToken();
    }
    while (binaryPriorityLeft(lexer->token) > limit) {
        int op = lexer->token;
        advanceToken();
        a = codeBinary(op, a, subExpr(binaryPriorityRight(op)));
    }
    return a;
}

Value Parser::expr() {
    return subExpr(0);
}
