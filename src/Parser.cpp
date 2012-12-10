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

extern __thread jmp_buf jumpBuf;

Func *Parser::parseFunc(const char *text) {
    SymbolTable syms;
    syms.set(hash64("ffi"), -(N_CONST_UPS + 1));
    syms.pushContext();
    Lexer lexer(text);

    if (int err = setjmp(jumpBuf)) {
        printf("err %d at line %d, before '%s'\n", err, lexer.lineNumber, lexer.p);
        return 0;
    }

    Proto *proto = Proto::alloc(0);
    Parser parser(proto, &syms, &lexer);
    int slot;
    Proto *funcProto = parser.parseProto(&slot);
    return makeFunc(funcProto, slot);
}

Func *Parser::parseStatList(const char *text) {
    SymbolTable syms;
    Proto *proto = Proto::alloc(0);
    syms.set(hash64("ffi"), -(N_CONST_UPS + 1));
    syms.pushContext();

    if (Parser::parseStatList(proto, &syms, text)) { return 0; }
    close(proto);
    return makeFunc(proto, -1);
}

Func *Parser::makeFunc(Proto *proto, int slot) {
    Value builtins[] = { VAL_OBJ(CFunc::alloc(ffiConstruct, 0)) };
    Value dummyRegs;
    return Func::alloc(proto, builtins + N_CONST_UPS + 1, &dummyRegs, slot);
}

int Parser::parseStatList(Proto *proto, SymbolTable *syms, const char *text) {
    Lexer lexer(text);
    if (int err = setjmp(jumpBuf)) {
        printf("at line %d, '%s'\n", lexer.lineNumber, lexer.p);
        return err;
    }
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
    case TK_FOR:   forStat(); break;

    case TK_NAME: 
        if (lexer->lookahead() == '=') {
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
        emit(proto->localsTop+3, SET, OA(code), VAL_REG(OB(code)), expr(proto->localsTop+2));
    }    
}

#define HERE (proto->code.size)

void Parser::forStat() {
    consume(TK_FOR);
    ERR(TOKEN != TK_NAME, E_FOR_NAME);
    u64 name = lexer->info.nameHash;
    int slot = proto->localsTop++;
    advance();
    consume(':'+TK_EQUAL);
    Value a = expr(slot+2);
    patchOrEmitMove(slot+2, slot+2, a);
    /*
    if (!IS_REG(a)) {
        ERR(!IS_INT(a), E_FOR_NOT_INT);
        s64 va = getInteger(a);
        if (va < -1 || va > 1) {
            emit(slot+2, MOVE, slot+2, a, UNUSED);
            a = VAL_REG(slot+2);
        }
    }
    */
    consume(':');
    Value b = expr(slot+3);
    patchOrEmitMove(slot+3, slot+1, b);
    int pos1 = emitHole();
    int pos2 = HERE;
    syms->set(name, slot);
    block();
    emitJump(HERE, LOOP, VAL_REG(slot), pos2);
    emitJump(pos1, FOR,  VAL_REG(slot), HERE);
}

void Parser::whileStat() {
    consume(TK_WHILE);
    unsigned pos1 = emitHole();
    unsigned pos2 = proto->code.size;
    Value a = expr(proto->localsTop);
    Vector<unsigned> copyExp;
    Vector<unsigned> &code = proto->code;
    while (code.size > pos2) {
        copyExp.push(code.pop());
    }
    block();
    emitJump(pos1, JMP, UNUSED, HERE);
    while (copyExp.size) {
        code.push(copyExp.pop());
    }
    emitJump(HERE, JT, a, pos2);
}

void Parser::ifStat() {
    consume(TK_IF);
    Value a = expr(proto->localsTop);
    int pos1 = emitHole();
    block();
    if (lexer->token == TK_ELSE) {
        int pos2 = emitHole();
        emitJump(pos1, JF, a, HERE);
        consume(TK_ELSE);
        if (lexer->token == '{') {
            block();
        } else { 
            ifStat();
        }
        emitJump(pos2, JMP, UNUSED, HERE);
    } else {
        emitJump(pos1, JF, a, HERE);
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
    proto->addUp(sym.slot);
    return syms->set(proto->level, name, -proto->nUp());
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
    int slot = top;
    
    Map *map = Map::alloc();
    emit(slot, ADD, slot, EMPTY_MAP, VAL_OBJ(map));

    for (int pos = 0; ; ++pos) {
        if (TOKEN == '}') { break; }
        Value k;
        if (TOKEN == TK_NAME && lexer->lookahead() == '=') {
            k = String::makeVal(lexer->info.name.buf, lexer->info.name.size-1);
            consume(TK_NAME);
            consume('=');
        } else {
            k = expr(top+1);
            consume(':');
        }
        Value v = expr(topAbove(k, top+1));
        
        if (IS_REG(k) || IS_REG(v)) {
            emit(top+2, SET, slot, k, v);
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
            emit(slot, SET, slot, VAL_NUM(pos), elem);
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
    bool indexOnly = false;
    bool callOnly  = false;
    switch (lexer->token) {
    case TK_INTEGER: a = VAL_NUM(lexer->info.intVal); advance(); return a;
    case TK_DOUBLE:  a = VAL_NUM(lexer->info.doubleVal); advance(); return a;
    case TK_NIL:     a = NIL; advance(); return a;

    case TK_NAME: {
        u64 name = lexer->info.nameHash;
        advance();
        a = VAL_REG(lookupSlot(name));
        proto->patchPos = -1;
        break;
    }

    case '(':
        advance();
        a = expr(top);
        consume(')');
        break;

    case TK_STRING:
        a = lexer->info.stringVal;
        advance();
        indexOnly = true;
        break;

    case '[': a = arrayExpr(top); indexOnly = true; break;
    case '{': a = mapExpr(top);   indexOnly = true; break;

    case TK_FUNC: a = funcExpr(top); callOnly = true; break;

    // case '?':
        
    default: ERR(true, E_SYNTAX);
    }

    while (true) {
        int t = TOKEN;
        if ((indexOnly && t != '[') || (callOnly && t != '(')) {
            return a;
        }
        indexOnly = callOnly = false;
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
            emit(0, CALL, base, a, VAL_NUM(nArg));
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

Proto *Parser::parseProto(int *outSlot) {
    consume(TK_FUNC);
    int slot = -1;
    if (TOKEN == TK_NAME) {
        u64 name = lexer->info.nameHash;
        advance();
        
        SymbolData s = syms->get(name);
        if (s.kind != KIND_EMPTY && s.level == proto->level && s.slot >= 0) {
            slot = s.slot;
        } else {
            slot = proto->localsTop++;
            syms->set(name, slot);
        }
    }

    proto = Proto::alloc(proto);
    syms->pushContext();
    parList();
    block();
    emit(0, RET, 0, NIL, UNUSED);
    proto->freeze();
    Proto *funcProto = proto;
    syms->popContext();
    proto = proto->up;
    *outSlot = slot;
    return funcProto;
}

Value Parser::funcExpr(int top) {
    int slot;
    Proto *funcProto = parseProto(&slot);
    emit(top+1, FUNC, top, VAL_OBJ(funcProto), VAL_REG(slot));
    return VAL_REG(top);
}

static Value doSub(Value a, Value b) {
    return BINOP(-, a, b);
}

static Value doMul(Value a, Value b) {
    return BINOP(*, a, b);
}

static Value doDiv(Value a, Value b) {
    return BINOP(/, a, b);
}

static Value foldUnary(int op, Value a) {
    if (!IS_REG(a)) {
        switch (op) {
        case '!': return IS_FALSE(a) ? TRUE : FALSE;
        case '-': return doSub(ZERO, a);
        case '~': return IS_NUM(a) ? VAL_NUM(~(unsigned)GET_NUM(a)) : ERROR(E_WRONG_TYPE);
        case '#': return IS_ARRAY(a) || IS_STRING(a) || IS_MAP(a) ? VAL_NUM(len(a)) : ERROR(E_WRONG_TYPE);
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
    case '~': opcode = XOR; b = VAL_NUM(-1); break;
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

    case TK_IS: opcode = IS; break;
    case TK_NOT_IS: opcode = NIS; break;

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
    case '^': return 12;
    case '*': case '/': case '%': return 10;

    case TK_SHIFT_L:
    case TK_SHIFT_R: return 9;

    case '+': case '-': return 8;


    case '&': return 7;
    case TK_BIT_XOR:
    case '|': return 6;

    case '='+TK_EQUAL: case '!'+TK_EQUAL:
    case '<': case '<'+TK_EQUAL: 
    case '>': case '>'+TK_EQUAL:
    case TK_IS: case TK_NOT_IS:
        return 5;

    case TK_LOG_AND: return 4;
    case TK_LOG_OR: return 2;

    default : return -1;
    }
}

Value Parser::subExpr(int top, int limit) {
    Value a;
    if (isUnaryOp(lexer->token)) {
        int op = lexer->token;
        advance();
        a = codeUnary(top, op, subExpr(top, 10));
    } else {
        a = suffixedExpr(top);
    }
    int op;
    int prio;
    while ((prio = binaryPriorityLeft(op = TOKEN)) > limit) {
        advance();
        const int rightPrio = op == '^' || op == TK_LOG_AND || op == TK_LOG_OR ? prio-1 : prio;
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
                    emitJump(pos1, JF, a, HERE);
                } else {
                    emitJump(pos1, JT, a, HERE);
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

Value Parser::ternaryExpr(int top) {
    Value a = subExpr(top, 0);
    if (TOKEN != '?') { 
        return a; 
    }
    advance();
    int pos1 = emitHole();
    Value b = ternaryExpr(top);
    patchOrEmitMove(top, top, b);
    int pos2 = emitHole();
    emitJump(pos1, JF, a, HERE);
    consume(':');
    Value c = ternaryExpr(top);
    patchOrEmitMove(top, top, c);
    emitJump(pos2, JMP, UNUSED, HERE);
    return VAL_REG(top);
}

Value Parser::expr(int top) {
    return ternaryExpr(top);
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

static bool isRangeInt(Value a, int min, int max) {
    if (!IS_NUM(a)) { return false; }
    double d = GET_NUM(a);
    int i = (int) d;
    return d==i && min <= i && i <= max;    
}

static bool isByteInt(Value a) {
    return isRangeInt(a, -128, 127);
}

static bool isShortInt(Value a) {
    return isRangeInt(a, -(1<<15), (1<<15)-1);
}

static Value mapSpecialConsts(Value a) {
    if (a == NIL) {
        return VAL_REG(UP_NIL);
    } else if (a == ZERO) {
        return VAL_REG(UP_ZERO);
    } else if (a == ONE) {
        return VAL_REG(UP_ONE);
    } else if (a == VAL_NUM(-1)) {
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
    assert(op == JMP || op == JF || op == JT || op == FOR || op == LOOP);
    const int offset = to - pos - 1;
    assert(offset != 0);
    proto->patchPos = -1;

    if ((op == JF && IS_FALSE(a)) || (op == JT && IS_TRUE(a))) {
        op = JMP;
        a  = UNUSED;
    } else if (!IS_REG(a)) {
        assert(op != FOR && op != LOOP);
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
            if (dest != (int)a) {
                proto->code.push(CODE_CAB(MOVE_R, dest, a, 0));
            }
        } else if (isShortInt(a)) {
            proto->code.push(CODE_CD(MOVE_I, dest, (short) GET_NUM(a)));
        } else {
            proto->code.push(CODE_CD(MOVE_C, dest, 0));
            proto->code.push((unsigned)a);
            proto->code.push((unsigned)(a>>32));
        }
        return;
    }

    if (op == CALL) {
        assert(IS_REG(a));
        assert(isByteInt(b));
        proto->code.push(CODE_CAB(CALL, dest, a, (int) GET_NUM(b)));
        return;
    }

    if (!IS_REG(a)) {
        emitCode(top+1, MOVE, top, a, UNUSED);
        a = VAL_REG(top);
        ++top;
    }
    if ((op == SHL || op == SHR) && isByteInt(b)) {
        proto->patchPos = proto->code.size;
        proto->code.push(CODE_CAB(op+1, dest, a, (int) GET_NUM(b)));
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
