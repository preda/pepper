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
#include <stdlib.h>

#define UNUSED VAL_REG(0)
#define TOKEN (lexer->token)

Parser::Parser(Proto *proto, SymbolTable *syms, Lexer *lexer) {
    this->proto = proto;
    this->syms  = syms;
    this->lexer = lexer;
    lexer->advance();
}

Parser::~Parser() {
}

void Parser::advance() {
    lexer->advance();
}

void Parser::consume(int t) {
    if (t == lexer->token) {
        lexer->advance();
    } else {
        ERR(true, E_EXPECTED + t);
    }
}

Func *Parser::parseFunc(const char *text) {
    Proto proto;
    SymbolTable syms;
    Lexer lexer(text);
    // Parser(&proto, &syms, &lexer).func();
    // TODO
    return 0;
}

int Parser::parseStatList(Proto *proto, SymbolTable *syms, const char *text) {    
    if (int err = catchError()) { return err; }
    Lexer lexer(text);
    Parser(proto, syms, &lexer).statList();
    return 0;
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
    case TK_VAR: varStat(); break;
    case TK_IF:  ifStat();   break;

    case TK_NAME: 
        if (lexer->lookahead() == '=') {
            assignStat();
        } else {
            exprOrAssignStat();
        }
        break;

    default: exprOrAssignStat(); break;
    }
}

void Parser::assignStat() {
    assert(TOKEN == TK_NAME);
    int slot = lookupSlot(lexer->info.nameHash);
    consume(TK_NAME);
    consume('=');
    patchOrEmitMove(slot, expr(proto->localsTop));
}

static Value makeValue(bool flag, byte ra) {
    return !flag ? VAL_REG(ra) :
        !(ra & 0x80) ? VAL_INT(((signed char)(ra<<1))>>1) :
        ra >= 0xf0 ? VALUE(ra & 0xf, 0) :
        VAL_REG(-((ra&0x7f) + 1));
}

static byte unCode(unsigned code, Value *c, Value *a, Value *b) {
    byte bigop = OP(code);
    *a = makeValue(bigop & 0x20, OA(code));
    *b = makeValue(bigop & 0x40, OB(code));
    *c = makeValue(bigop & 0x80, OC(code));
    return bigop & 0x1f;
}

static int max(int a, int b) { return a < b ? b : a; }

static int topAbove(Value a, int top) {
    if (!IS_REGISTER(a)) {
        return top;
    }
    return max((int)a + 1, top);
}

// void printValue(Value a);

void Parser::exprOrAssignStat() {
    Value lhs = expr(proto->localsTop);
    if (TOKEN == '=') {
        consume('=');
        ERR(!IS_REGISTER(lhs), E_ASSIGN_TO_CONST);
        Value a, b, c;
        int op = unCode(proto->code.pop(), &c, &a, &b);
        // printValue(c); printValue(a); printValue(b);
        
        ERR(op != GET, E_ASSIGN_RHS);
        assert(lhs == c);
        emitCode(makeCode(SET, a, b, expr(topAbove(lhs, proto->localsTop))));
    }    
}

void Parser::ifStat() {
    consume(TK_IF);
    Value a = expr(proto->localsTop);
    int pos1 = emitHole();
    block();
    if (lexer->token == TK_ELSE) {
        int pos2 = emitHole();
        emitPatchJumpHere(pos1, a);
        consume(TK_ELSE);
        if (lexer->token == '{') {
            block();
        } else { 
            ifStat();
        }
        emitPatchJumpHere(pos2, ZERO);
    } else {
        emitPatchJumpHere(pos1, a);
    }
}

void Parser::varStat() {
    consume(TK_VAR);
    if (TOKEN == TK_NAME) {
        u64 name = lexer->info.nameHash;
        consume(TK_NAME);
        int top = proto->localsTop;
        Value a = NIL;
        if (TOKEN == '=') {
            consume('=');
            a = expr(top);
        }
        syms->set(name, top);
        patchOrEmitMove(top, a);
        ++proto->localsTop;
    } else {
        ERR(true, E_VAR_NAME);
    }
}

static bool isUnaryOp(int token) {
    return token=='!' || token=='-' || token=='#' || token=='~';
}

static Value foldUnary(int op, Value a) {
    if (!IS_REGISTER(a)) {
        switch (op) {
        case '!': return IS_FALSE(a) ? TRUE : FALSE;
        case '-': return doSub(ZERO, a);
        case '~': return doXor(a, VAL_INT(-1));
        case '#': return IS_ARRAY(a) || IS_STRING(a) || IS_MAP(a) ? VAL_INT(len(a)) : ERROR(E_WRONG_TYPE);
        }
    }
    return NIL;
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

Value Parser::codeUnary(int top, int op, Value a) {
    {
        Value c = foldUnary(op, a);
        if (c != NIL) { return c; }
    }
    Value b = UNUSED;
    int opcode = 0;
    switch (op) {
    case '!': opcode = NOT; break;
    case '-': opcode = SUB; b = a; a = ZERO; break;
    case '~': opcode = XOR; b = VAL_INT(-1); break;
    case '#': opcode = LEN; break;
    default: assert(false);
    }
    Value c = VAL_REG(top);
    emitCode(makeCode(opcode, c, a, b));
    return c;
}

Value Parser::codeBinary(int top, int op, Value a, Value b) {
    {
        Value c = foldBinary(op, a, b);
        if (c != NIL) { return c; }
    }
    int opcode = 0;
    switch (op) {
    case '+': opcode = ADD; break;
    case '-': opcode = SUB; break;
    case '*': opcode = MUL; break;
    case '/': opcode = DIV; break;
    case '%': opcode = MOD; break;
    case '^': opcode = POW; break;
    default: assert(false);
    }
    Value c = VAL_REG(top);
    emitCode(makeCode(opcode, c, a, b));
    return c;
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

 int Parser::lookupSlot(u64 name) {
     SymbolData sym = lookupName(name);
     ERR(sym.kind == KIND_EMPTY, E_NAME_NOT_FOUND);
     return sym.slot;
 }

Value Parser::primaryExpr(int top) {
    printf("enter primaryExpr\n");
    Value a = NIL;
    switch (lexer->token) {
    case TK_NAME: {
        u64 name = lexer->info.nameHash;
        consume(TK_NAME);        
        return VAL_REG(lookupSlot(name));
    }

    case '(':
        consume('(');
        a = expr(top);
        consume(')');
        return a;
        
    case TK_STRING:
        a = VAL_STRING(lexer->info.strVal, lexer->info.strLen);
        consume(TK_STRING);
        return a;

    case '[':
        return arrayExpr(top);

    case '{':
        return mapExpr(top);

    default:
        ERR(true, E_SYNTAX);            
    }
}

Value Parser::mapExpr(int top) {
    consume('{');
    if (TOKEN=='}') {
        consume('}');
        return EMPTY_MAP;
    }
    int slot = top++;
    int codePosInit = emitHole();

    Vector<Value> keys;
    Vector<Value> vals;
    int nConsts = 0;

    for (int pos = 0; ; ++pos) {
        if (TOKEN == '}') { break; }
        Value k = expr(top);
        consume(':');
        Value v = expr(top);
        
        if (IS_REGISTER(k) || IS_REGISTER(v) || (k==NIL && v == NIL)) {
            keys.push(NIL);
            vals.push(NIL);
            emitCode(makeCode(SET, VAL_REG(slot), k, v));
        } else {
            keys.push(k);
            vals.push(v);
            ++nConsts;
        }
        if (TOKEN == '}') { break; }
        consume(',');
    }
    if (nConsts <= 2) {
        int pos = 0;
        for (Value *pk = keys.buf, *endk = pk+keys.size, *pv = vals.buf; 
             pk < endk; ++pk, ++pv, ++pos) {
            Value k = *pk;
            Value v = *pv;
            if (k != NIL || v != NIL) {
                emitCode(makeCode(SET, VAL_REG(slot), k, v));
            }
        }
        emitPatch(codePosInit, makeCode(MOVE, VAL_REG(slot), EMPTY_MAP, UNUSED));
    } else {
        emitPatch(codePosInit, makeCode(ADD,  VAL_REG(slot), EMPTY_MAP, VAL_OBJ(Map::alloc(&keys, &vals))));
    }
    consume('}');
    return VAL_REG(slot);
}

Value Parser::arrayExpr(int top) {
    consume('[');
    if (TOKEN==']') {
        consume(']');
        return EMPTY_ARRAY;
    }
    int slot = top++;
    int codePosInit = emitHole();

    Vector<Value> vect;
    int nConsts = 0;
    for (int pos = 0; ; ++pos) {
        if (TOKEN == ']') { break; }
        Value elem = expr(top);
        if (IS_REGISTER(elem) || elem==NIL) {
            vect.push(NIL);
            emitCode(makeCode(SET, VAL_REG(slot), VAL_INT(pos), elem));
        } else {
            vect.push(elem);
            ++nConsts;
        }
        if (TOKEN == ']') { break; }
        consume(',');
    }
    if (nConsts <= 2) {
        int pos = 0;
        for (Value *p = vect.buf, *end = p+vect.size; p < end; ++p, ++pos) {
            Value v = *p;
            if (v != NIL) {
                emitCode(makeCode(SET, VAL_REG(slot), VAL_INT(pos), v));
            }
        }
        emitPatch(codePosInit, makeCode(MOVE, VAL_REG(slot), EMPTY_ARRAY, UNUSED));
    } else {
        emitPatch(codePosInit, makeCode(ADD, VAL_REG(slot), EMPTY_ARRAY, VAL_OBJ(Array::alloc(&vect))));
    }
    consume(']');
    return VAL_REG(slot);
}

Value Parser::suffixedExpr(int top) {
    Value a = primaryExpr(top);
    while (true) {
        switch(TOKEN) {
        case '[':
            consume('[');            
            emitCode(makeCode(GET, VAL_REG(top), a, expr(topAbove(a, top))));
            a = VAL_REG(top);
            consume(']');
            break;

        default:
            printf("exit suffixed %d\n", TOKEN);
            return a;
        }
    }
}

Value Parser::simpleExpr(int top) {
    // printf("enter simpleExpr\n");
    Value ret = NIL;
    switch (lexer->token) {
    case TK_INTEGER: ret = VAL_INT(lexer->info.intVal); break;
    case TK_DOUBLE:  ret = VAL_DOUBLE(lexer->info.doubleVal); break;
    case TK_NIL:     ret = NIL; break;

    case TK_FUNC: ERR(true, E_TODO);
        break;

    default:
        return suffixedExpr(top);
    }
    advance();
    return ret;
}

Value Parser::subExpr(int top, int limit) {
    // printf("enter subExpr %d\n", limit);
    Value a;
    if (isUnaryOp(lexer->token)) {
        int op = lexer->token;
        advance();
        a = codeUnary(top, op, subExpr(top, 8));
    } else {
        a = simpleExpr(top);
    }
    while (binaryPriorityLeft(lexer->token) > limit) {
        int op = lexer->token;
        advance();
        a = codeBinary(top, op, a, subExpr(topAbove(a, top), binaryPriorityRight(op)));
    }
    return a;
}

Value Parser::expr(int top) {
    return subExpr(top, 0);
}


// code generation below

void Parser::patchOrEmitMove(int dest, Value src) {
    if (IS_REGISTER(src)) {
        int srcSlot = (int) src;        
        if (srcSlot == dest) { return; } // everything is in the right place, do nothing

        unsigned code = *proto->code.top();
        Value a, b, c;
        int op = unCode(code, &c, &a, &b);
        if (opcodeHasDest(op)) {
            assert(IS_REGISTER(c));
            if (srcSlot == (int) c) {
                proto->code.pop();
                emitCode(makeCode(op, VAL_REG(dest), a, b));
                return; // last instruction patched
            }
        }
    }
    emitCode(makeCode(MOVE, VAL_REG(dest), src, UNUSED));
}

Value Parser::maybeAllocConst(Value a) {
    int ta = TAG(a);
    if (ta==REGISTER || ta==ARRAY || ta==MAP || ta==STRING || a==NIL ||
        (ta==INTEGER && getInteger(a)>=-64 && getInteger(a)<64)) {
        return a;
    }
    proto->ups.push(0);
    proto->consts.push(a);
    return VAL_REG(- proto->ups.size);
}

byte Parser::getRegValue(Value a) {
    a = maybeAllocConst(a);
    const int ta = TAG(a);
    return
        ta==REGISTER && (int)a >= 0 ? (int) a :
        ta==REGISTER ? 0x80 | (-(int)a-1) :
        ta==ARRAY || ta==MAP || ta==STRING || a==NIL? (0xf0 | ta) : ((unsigned)a & 0x7f);
}

static inline unsigned PACK4(unsigned b0, unsigned b1, unsigned b2, unsigned b3) {
    return b0 | (b1<<8) | (b2<<16) | (b3<<24);
}

static bool isNormal(Value a) {
    return IS_REGISTER(a) && (int)a >= 0;
}

static byte flags(Value a, Value b, Value c) {
    return (isNormal(a)?0:0x20) | (isNormal(b)?0:0x40) | (isNormal(c)?0:0x80);
}

unsigned Parser::makeCode(int op, Value c, Value a, Value b) {
    return PACK4(op | flags(a, b, c), getRegValue(a), getRegValue(b), getRegValue(c));
}

void Parser::emitCode(unsigned code) {
    proto->code.push(code);
}

int Parser::emitHole() {
    return (int) proto->code.push(0);
}

void Parser::emitPatch(unsigned pos, unsigned code) {
    assert(pos < proto->code.size);
    proto->code.buf[pos] = code;
}

void Parser::emitPatchJumpHere(unsigned pos, Value cond) {
    int offset = proto->code.size - pos - 1;
    emitPatch(pos, makeCode(JMP, UNUSED, VAL_INT(offset), cond));
}
