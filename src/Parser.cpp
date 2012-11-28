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
#include "CFunc.h"
#include "FFI.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <setjmp.h>

#define UNUSED      VAL_REG(0)

static const Value EMPTY_ARRAY = VAL_OBJ(Array::alloc());
static const Value EMPTY_MAP   = VAL_OBJ(Map::alloc());

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
    /*
    Proto proto;
    SymbolTable syms;
    Lexer lexer(text);
    */
    // Parser(&proto, &syms, &lexer).func();
    // TODO
    return 0;
}

Func *Parser::parseStatList(const char *text) {
    SymbolTable syms;
    Proto *proto = Proto::alloc(0);
    syms.set(hash64("ffi"), -8);
    proto->ups.push(-8);
    if (Parser::parseStatList(proto, &syms, text)) { return 0; }
    close(proto);
    return Func::alloc(proto, 0, 0);
}

extern __thread jmp_buf jumpBuf;

int Parser::parseStatList(Proto *proto, SymbolTable *syms, const char *text) {
    if (int err = setjmp(jumpBuf)) {
        return err;
    }
    Lexer lexer(text);
    Parser parser(proto, syms, &lexer);
    parser.statList();
    return 0;
}

void Parser::block() {
    consume('{');
    statList();
    consume('}');
}

void Parser::statList() {
    while (lexer->token != '}' && lexer->token != TK_END) {
        statement();
    }
}

void Parser::statement() {
    switch (lexer->token) {
    case TK_VAR:   advance(); varStat(); break;
    case TK_IF:    ifStat(); break;
    case TK_WHILE: whileStat(); break;

    case TK_NAME: 
        if (lexer->lookahead() == '=') {
            // assert(TOKEN == TK_NAME);
            int slot = lookupSlot(lexer->info.nameHash);
            advance();
            consume('=');
            patchOrEmitMove(proto->localsTop+1, slot, expr(proto->localsTop));
            proto->patchPos = -1;
        } else if (lexer->lookahead() == ':'+TK_EQUAL) { 
            varStat();
        } else {
            exprOrAssignStat();
        }
        break;

    case TK_RETURN:
        advance();
        emit(proto->localsTop, RET, 0, TOKEN == ';' ? NIL : expr(proto->localsTop), UNUSED);
        break;        

    default: exprOrAssignStat(); break;
    }
    while (TOKEN == ';') { advance(); }
}

static int topAbove(Value a, int top) {
    return IS_REG(a) ? max((int)a + 1, top) : top;
}

void Parser::exprOrAssignStat() {
    Value lhs = expr(proto->localsTop);
    if (TOKEN == '=') {
        consume('=');
        ERR(!IS_REG(lhs), E_ASSIGN_TO_CONST);
        ERR(proto->patchPos < 0, E_ASSIGN_RHS);        
        unsigned code = proto->code.pop();        
        ERR(OP(code) != GET, E_ASSIGN_RHS); // (lhs & FLAG_DONT_PATCH)
        assert((int)lhs == OC(code));
        emit(proto->localsTop+1, SET, OA(code), VAL_REG(OB(code)), expr(topAbove(lhs, proto->localsTop)));
    }    
}

#define HERE (proto->code.size)
void Parser::whileStat() {
    consume(TK_WHILE);
    int pos1 = HERE;
    Value a = expr(proto->localsTop);
    int pos2 = emitHole();
    block();
    emitJump(HERE, JMP, UNUSED, pos1);
    emitJump(pos2, JMPF, a, HERE);
}

void Parser::ifStat() {
    consume(TK_IF);
    Value a = expr(proto->localsTop);
    int pos1 = emitHole();
    block();
    if (lexer->token == TK_ELSE) {
        int pos2 = emitHole();
        emitJump(pos1, JMPF, a, HERE);
        consume(TK_ELSE);
        if (lexer->token == '{') {
            block();
        } else { 
            ifStat();
        }
        emitJump(pos2, JMP, UNUSED, HERE);
    } else {
        emitJump(pos1, JMPF, a, HERE);
    }
}

void Parser::varStat() {
    ERR(TOKEN != TK_NAME, E_VAR_NAME);
    u64 name = lexer->info.nameHash;
    advance();
    int top = proto->localsTop;
    Value a = NIL;
    int aSlot = -1;
    if (TOKEN == '=' || TOKEN == ':'+TK_EQUAL) {
        advance();
        a = expr(top);
        SymbolData s = syms->get(name);
        if (s.kind != KIND_EMPTY && s.level == proto->level && s.slot >= 0) {
            aSlot = s.slot; // reuse existing local with same name
        }
    } else {
        SymbolData s = lookupName(name);
        if (s.kind != KIND_EMPTY) {
            if (s.slot < 0) {
                a = VAL_REG(s.slot); // init from upval with same name
            } else {
                return; // reuse existing local unchanged
            }
        }
    }
    if (aSlot < 0) {
        aSlot = proto->localsTop++;
        syms->set(name, aSlot);
    }
    patchOrEmitMove(proto->localsTop+1, aSlot, a);
    proto->patchPos = -1;
}

static bool isUnaryOp(int token) {
    return token=='!' || token=='-' || token=='#' || token=='~';
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

Value Parser::mapExpr(int top) {
    consume('{');
    if (TOKEN=='}') {
        consume('}');
        return EMPTY_MAP;
    }
    int slot = top++;
    
    Map *map = Map::alloc();
    emit(slot, ADD, slot, EMPTY_MAP, VAL_OBJ(map));

    for (int pos = 0; ; ++pos) {
        if (TOKEN == '}') { break; }
        Value k = expr(top);
        consume(':');
        Value v = expr(top);
        
        if (IS_REG(k) || IS_REG(v)) {
            emit(slot, SET, slot, k, v);
        } else {
            map->set(k, v);
        }
        if (TOKEN == '}') { break; }
        consume(',');
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
    Array *array = Array::alloc();
    emit(slot, ADD, slot, EMPTY_ARRAY, VAL_OBJ(array));

    for (int pos = 0; ; ++pos) {
        if (TOKEN == ']') { break; }
        Value elem = expr(top);
        if (IS_REG(elem)) {
            emit(slot, SET, slot, VAL_INT(pos), elem);
        } else {
            array->push(elem);
        }
        if (TOKEN == ']') { break; }
        consume(',');
    }
    consume(']');
    return VAL_REG(slot);
}

#define ARG_LIST(element) if (TOKEN != ')') { while (true) { element; if (TOKEN == ')') { break; } consume(','); } }

void Parser::parList() {
    consume('(');
    ARG_LIST(if (TOKEN == TK_NAME) {
            ++proto->nArgs;
            syms->set(lexer->info.nameHash, proto->localsTop++);
        }
        consume(TK_NAME););
    consume(')');
}

Value Parser::suffixedExpr(int top) {
    Value a = NIL;
    const char *restrict = 0;
    switch (lexer->token) {
    case TK_INTEGER: a = VAL_INT(lexer->info.intVal); advance(); return a;
    case TK_DOUBLE:  a = VAL_DOUBLE(lexer->info.doubleVal); advance(); return a;
    case TK_NIL:     a = NIL; advance(); return a;

    case TK_NAME: {
        u64 name = lexer->info.nameHash;
        advance();
        a = VAL_REG(lookupSlot(name));
        break;
    }

    case '(':
        advance();
        a = expr(top);
        // if (IS_REG(a) && (int)a == top) { ++top; }
        consume(')');
        break;

    case TK_STRING:
        a = lexer->info.stringVal;
        advance();
        restrict = "[";
        break;

    case '[': a = arrayExpr(top); restrict = "["; break;
    case '{': a = mapExpr(top);   restrict = "["; break;

    case TK_FUNC: a = funcExpr(top); restrict = "("; break;

    default: ERR(true, E_SYNTAX);
    }

    while (true) {
        int t = TOKEN;
        if (restrict && !strchr(restrict, t)) {
            return a;
        }
        switch(t) {
        case '[':
            advance();
            emit(top+2, GET, top, a, expr(top+1));
            a = VAL_REG(top);
            consume(']');
            break;

        case '(': {
            advance();
            int base = top;
            ERR(!IS_REG(a), E_CALL_NOT_FUNC);
            if (IS_REG(a) && (int)a == base) { ++base; }
            int nArg = 0;
            ARG_LIST(int argPos = base + nArg;
                     patchOrEmitMove(argPos, argPos, expr(argPos));
                     ++nArg;);
            consume(')');
            emit(0, CALL, base, a, VAL_INT(nArg));
            if (base != top) {
                emit(base+1, MOVE, top, VAL_REG(base), UNUSED);
            }
            a = VAL_REG(top);
            break;
        }

        default: return a;
        }
    }
}

Value Parser::funcExpr(int top) {
    proto = Proto::alloc(proto);
    syms->pushContext();
    consume(TK_FUNC);
    parList();
    block();
    emit(proto->localsTop, RET, 0, NIL, UNUSED);
    proto->freeze();
    syms->popContext();
    Proto *funcProto = proto;
    proto = proto->up;
    emit(top+1, FUNC, top, VAL_OBJ(funcProto), UNUSED);
    return VAL_REG(top);
}

static Value foldUnary(int op, Value a) {
    if (!IS_REG(a)) {
        switch (op) {
        case '!': return IS_FALSE(a) ? TRUE : FALSE;
        case '-': return doSub(ZERO, a);
        case '~': return IS_INT(a) ? VAL_INT(~getInteger(a)) : ERROR(E_WRONG_TYPE);
        case '#': return IS_ARRAY(a) || IS_STRING(a) || IS_MAP(a) ? VAL_INT(len(a)) : ERROR(E_WRONG_TYPE);
        }
    }
    return NIL;
}

static Value foldBinary(int op, Value a, Value b) {
    if (!IS_REG(a) && !IS_REG(b)) {
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
    case '!': opcode = NOTL; break;
    case '-': opcode = SUB; b = a; a = ZERO; break;
    case '~': opcode = XOR; b = VAL_INT(-1); break;
    case '#': opcode = LEN; break;
    default: assert(false);
    }
    emit(top+1, opcode, top, a, b);
    return VAL_REG(top);
}

Value Parser::codeBinary(int top, int op, Value a, Value b) {
    {
        Value c = foldBinary(op, a, b);
        if (c != NIL) { return c; }
    }
    Value c;
    int opcode = 0;
    switch (op) {
        // arithmetic
    case '+': opcode = ADD; break;
    case '-': opcode = SUB; break;
    case '*': opcode = MUL; break;
    case '/': opcode = DIV; break;
    case '%': opcode = MOD; break;
    case '^': opcode = POW; break;

        // bit operations
    case '&': opcode = AND; break;
    case '|': opcode = OR;  break;

    case TK_BIT_XOR: opcode = XOR; break;
    case TK_SHIFT_L: opcode = SHL; break;
    case TK_SHIFT_R: opcode = SHR; break;

    case '<':          opcode = LT; break;
    case '<'+TK_EQUAL: opcode = LE; break;

    case '>':          c=a; a=b; b=c; opcode = LT; break;
    case '>'+TK_EQUAL: c=a; a=b; b=c; opcode = LE; break;

    case '='+TK_EQUAL: opcode = EQ; break;
    case '!'+TK_EQUAL: opcode = NEQ; break;

    case TK_LOG_AND:
        break;

    case TK_LOG_OR:
        break;

    default: assert(false);
    }
    emit(topAbove(a, topAbove(b, top+1)), opcode, top, a, b);
    return VAL_REG(top);
}

static int binaryPriorityLeft(int token) {
    switch (token) {
    case '^': return 10;
    case '*': case '/': case '%': return 8;

    case TK_SHIFT_L:
    case TK_SHIFT_R: return 7;

    case '+': case '-': return 6;


    case '&': return 5;
    case TK_BIT_XOR:
    case '|': return 4;

    case '='+TK_EQUAL: case '!'+TK_EQUAL:
    case '<': case '<'+TK_EQUAL: 
    case '>': case '>'+TK_EQUAL: 
        return 3;

    case TK_LOG_AND: return 2;
    case TK_LOG_OR: return 1;

    default : return -1;
    }
}

/*
static int binaryPriorityRight(int token) {
    int left = binaryPriorityLeft(token);
    return token == '^' ? left-1 : left;
}
*/

Value Parser::subExpr(int top, int limit) {
    Value a;
    if (isUnaryOp(lexer->token)) {
        int op = lexer->token;
        advance();
        a = codeUnary(top, op, subExpr(top, 8));
    } else {
        a = suffixedExpr(top);
    }
    int op;
    int prio;
    while ((prio = binaryPriorityLeft(op = TOKEN)) > limit) {
        advance();
        const int rightPrio = op=='^' ? prio-1 : prio;
        if (op == TK_LOG_AND || op == TK_LOG_OR) {
            if (!IS_REG(a)) {
                int saveCodeSize = proto->code.size;
                Value b = subExpr(top, rightPrio);
                if ((op==TK_LOG_AND && IS_FALSE(a)) ||
                    (op==TK_LOG_OR  && !IS_FALSE(a))) {
                    // drop b
                    proto->code.size = saveCodeSize;                    
                } else {
                    a = b;
                }
            } else {
                int aSlot = (int) a;
                if (aSlot < top) {
                    emit(0, MOVE, top, a, UNUSED);
                    a = VAL_REG(top);
                    aSlot = top;
                }
                int pos1 = emitHole();
                Value b = subExpr(aSlot, rightPrio);
                patchOrEmitMove(aSlot, aSlot, b);
                if (op == TK_LOG_AND) {
                    emitJump(pos1, JMPF, a, HERE);
                } else {
                    emitJump(pos1, JMPT, a, HERE);
                }
                proto->patchPos = -1;
                // a |= FLAG_DONT_PATCH;
            }
        } else {
            a = codeBinary(top, op, a, subExpr(topAbove(a, top), rightPrio));
        }
    }
    return a;
}

Value Parser::expr(int top) {
    return subExpr(top, 0);
}


// code generation below

void Parser::patchOrEmitMove(int top, int dest, Value src) {
    int patchPos = proto->patchPos;
    assert(patchPos == -1 || patchPos == (int)proto->code.size-1 || patchPos == (int)proto->code.size-3);
    if (dest >= 0 && IS_REG(src) && (int)src >= 0 && patchPos >= 0) {
        int srcSlot = (int) src;
        unsigned code = proto->code.buf[patchPos];
        int op = OP(code);
        if (opcodeHasDest(op)) {
            int oldDest = OC(code);
            assert(srcSlot == oldDest);        
            if (oldDest == dest) { return; } // everything is in the right place
            proto->code.buf[patchPos] = CODE_CAB(OP(code), dest, OA(code), OB(code));
            return; // patched
        }
    }
    emit(top, MOVE, dest, src, UNUSED);
}

/*
static bool isNormalReg(Value a) {
    return IS_REG(a) && (int)a >= 0;
}
*/

enum {
    UP_NIL  = -1,
    UP_ZERO = -2,
    UP_ONE  = -3,
    UP_NEG_ONE = -4,
    UP_EMPTY_STRING = -5,
    UP_EMPTY_ARRAY  = -6,
    UP_EMPTY_MAP    = -7,
};

void Parser::close(Proto *proto) {
    proto->code.push(CODE_CAB(RET, 0, UP_NIL, 0));
    proto->freeze();
}

static bool isSmallInt(Value a) {
    s64 v;
    return IS_INT(a) && -128 <= (v=getInteger(a)) && v < 128;
}

static Value mapSpecialConsts(Value a) {
    if (a == NIL) {
        return VAL_REG(UP_NIL);
    } else if (a == VAL_INT(0)) {
        return VAL_REG(UP_ZERO);
    } else if (a == VAL_INT(1)) {
        return VAL_REG(UP_ONE);
    } else if (a == VAL_INT(-1)) {
        return VAL_REG(UP_NEG_ONE);
    } else if (a == EMPTY_STRING) {
        return VAL_REG(UP_EMPTY_STRING);
    } else if (a == EMPTY_ARRAY) {
        return VAL_REG(UP_EMPTY_ARRAY);
    } else if (a == EMPTY_MAP) {
        return VAL_REG(UP_EMPTY_MAP);
    }
    return a;
}

void Parser::emitJump(unsigned pos, int op, Value a, unsigned to) {
    assert(to <= proto->code.size);
    assert(pos  <= proto->code.size);
    assert(op == JMP || op == JMPF || op == JMPT);
    const int offset = to - pos - 1;
    assert(offset != 0);
    proto->patchPos = -1;
    if ((op == JMPF && IS_FALSE(a)) || (op == JMPT && IS_TRUE(a))) {
        op = JMP;
        a  = UNUSED;
    } else if (!IS_REG(a)) {
        return; // never jump == no-op
    }
    if (op == JMP) { assert(a == UNUSED); }
    proto->code.set(pos, CODE_CD(op, a, offset));
}

void Parser::emit(unsigned top, int op, int dest, Value a, Value b) {
    if (a == EMPTY_ARRAY || a == EMPTY_MAP) {
        if (op == MOVE) {
            assert(b == UNUSED);
            op = ADD;
            b = a;
        } else if (op == RET) {
            assert(b == UNUSED);
            emit(top, ADD, top, a, a);
            a = VAL_REG(top);
        }
    }
    if (op == SET && (dest == UP_EMPTY_ARRAY || dest == UP_EMPTY_MAP)) {
        proto->patchPos = -1;
        return;
    }
    a = mapSpecialConsts(a);
    if (op != CALL && op != SHL && op != SHR) {
        b = mapSpecialConsts(b);
    }
    emitCode(top, op, dest, a, b);
}

void Parser::emitCode(unsigned top, int op, int dest, Value a, Value b) {
    // assert(dest >= 0);
    proto->patchPos = proto->code.size;

    if (dest < 0) {
        if (op == MOVE && IS_REG(a)) {
            proto->code.push(CODE_CAB(MOVEUP, dest, a, 0));
            return;
        } else if (opcodeHasDest(op)) {
            emitCode(top+1, op, top, a, b);
            emitCode(top+1, MOVE, dest, VAL_REG(top), UNUSED);
            return;
        } else {
            dest = (byte) dest;
        }
    }

    if (op == MOVE) {
        assert(b == UNUSED);
        if (IS_REG(a)) {
            proto->code.push(CODE_CAB(MOVE_R, dest, a, 0));
        } else if (IS_INT(a) && getInteger(a) >= -(1<<15) && getInteger(a) < (1<<15)) {
            proto->code.push(CODE_CD(MOVE_I, dest, a));
        } else {
            proto->code.push(CODE_CD(MOVE_C, dest, 0));
            proto->code.push((unsigned)a);
            proto->code.push((unsigned)(a>>32));
        }
        return;
    }

    if (op == CALL) {
        assert(IS_REG(a));
        assert(isSmallInt(b));
        proto->code.push(CODE_CAB(CALL, dest, a, b));
        return;
    }

    if (!IS_REG(a)) {
        emitCode(top+1, MOVE, top, a, UNUSED);
        a = VAL_REG(top);
        ++top;
    }
    if ((op == SHL || op == SHR) && isSmallInt(b)) {
        proto->patchPos = proto->code.size;
        proto->code.push(CODE_CAB(op+1, dest, a, b));
        return;
    }
    if (!IS_REG(b)) {
        emitCode(top+1, MOVE, top, b, UNUSED);
        b = VAL_REG(top);
    }
    proto->patchPos = proto->code.size;
    proto->code.push(CODE_CAB(op, dest, a, b));
}

int Parser::emitHole() {
    proto->patchPos = -1;
    return (int) proto->code.push(0);
}
