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
#include "Pepper.h"
#include "GC.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <setjmp.h>

#define UNUSED VAL_REG(0)
#define TOKEN (lexer->token)

enum {
    UP_NIL  = -1,
    UP_ZERO = -2,
    UP_ONE  = -3,
    UP_NEG_ONE = -4,
    UP_EMPTY_STRING = -5,
    UP_EMPTY_ARRAY  = -6,
    UP_EMPTY_MAP    = -7,
};

Parser::Parser(GC *gc, Proto *proto, SymbolTable *syms, Lexer *lexer) :
    proto(proto),
    syms(syms),
    lexer(lexer),
    gc(gc)
{
    syms->enterBlock(true);
    lexer->advance();
}

Parser::~Parser() {
    syms->exitBlock(true);
}

void Parser::advance() {
    lexer->advance();
}

void Parser::consume(int t) {
    if (t == lexer->token) {
        lexer->advance();
    } else {
        CERR(true, E_EXPECTED + t, VNIL);
    }
}

extern __thread jmp_buf jumpBuf;

Func *Parser::parseInEnv(Pepper *pepper, const char *text, bool isFunc) {
    GC *gc = pepper->gc();
    SymbolTable *syms = pepper->syms();
    Value *regs = pepper->regs();
    regs[6] = Map::makeMap(gc, "func",  regs[0], "block", regs[1], NULL);
    regs[7] = Map::makeMap(gc, "read",  regs[2], NULL);
    regs[8] = Map::makeMap(gc, "class", regs[3], NULL);
    
    Func *f = isFunc ? 
        parseFunc(gc, syms, regs, text) :
        parseStatList(gc, syms, regs, text);
    return f;
}

Func *Parser::parseFunc(GC *gc, SymbolTable *syms, Value *regs, const char *text) {
    Lexer lexer(gc, text);

    if (int err = setjmp(jumpBuf)) {
        printf("err %d at line %d, before '%s'\n", err, lexer.lineNumber, lexer.p);
        return 0;
    }

    Proto *proto = Proto::alloc(gc, 0);
    Parser parser(gc, proto, syms, &lexer);
    int slot;
    Proto *funcProto = parser.parseProto(&slot);
    return Func::alloc(gc, funcProto, 0, regs, slot);
}

Func *Parser::parseStatList(GC *gc, SymbolTable *syms, Value *regs, const char *text) {
    Proto *proto = Proto::alloc(gc, 0);
    if (Parser::parseStatList(gc, proto, syms, text)) { return 0; }
    return Func::alloc(gc, proto, 0, regs, -1);
}

int Parser::parseStatList(GC *gc, Proto *proto, SymbolTable *syms, const char *text) {
    Lexer lexer(gc, text);
    if (int err = setjmp(jumpBuf)) {
        return err;
    }
    Parser parser(gc, proto, syms, &lexer);
    bool hasReturn = parser.statList();
    if (!hasReturn) {
        proto->code.push(CODE_CAB(RET, 0, UP_NIL, 0));
    }
    proto->freeze();
    return 0;
}

bool Parser::block() {
    syms->enterBlock(false);
    bool ret = insideBlock();
    syms->exitBlock(false);
    return ret;
}

bool Parser::insideBlock() {
   consume('{');
   bool ret = statList();
   consume('}');
   return ret;
}

bool Parser::statList() {
    bool endsWithReturn = false;
    while (lexer->token != '}' && lexer->token != TK_END) {
        endsWithReturn = statement();
    }
    return endsWithReturn;
}

// e.g. fn(x) => x+1
bool Parser::lambdaBody() {
    consume('=');
    consume('>');
    int top = syms->localsTop();
    emit(top, RET, 0, expr(top), UNUSED);
    return true;
}

bool Parser::statement() {
    bool isReturn = false;
    switch (lexer->token) {
    case '{': isReturn = block(); break;
    case TK_if:    ifStat(); break;
    case TK_while: whileStat(); break;
    case TK_for:   forStat(); break;
    case TK_return: {
        advance();
        int top = syms->localsTop();
        emit(top, RET, 0, expr(top), UNUSED);
        isReturn = true;
        break;        
    }

    case TK_NAME: {
        int lookahead = lexer->lookahead();
        if (lookahead == '=' || lookahead == ':'+TK_EQUAL) {
            Value name = lexer->info.name;
            consume(TK_NAME);
            if (lookahead == '=') {
                int slot = lookupSlot(name);
                consume('=');
                int top = syms->localsTop();
                patchOrEmitMove(top + 1, slot, expr(top));
                proto->patchPos = -1;                
            } else {
                consume(':'+TK_EQUAL); 
                if (syms->definedInThisBlock(name)) {
                    CERR(true, E_VAR_REDEFINITION, name);
                    // aSlot = slot; // reuse existing local with same name
                } else {
                    const Value a = expr(syms->localsTop());
                    const int slot = syms->set(name);
                    patchOrEmitMove(slot+1, slot, a);
                    proto->patchPos = -1;
                }
            }
            break;
        }
    }
        
    default: {
        int top = syms->localsTop();
        Value lhs = expr(top);
        if (TOKEN == '=') {
            consume('=');
            CERR(!IS_REG(lhs), E_ASSIGN_TO_CONST, lhs);
            CERR(proto->patchPos < 0, E_ASSIGN_RHS, lhs);
            unsigned code = proto->code.pop();
            int op = OP(code);
            CERR(op != GETI && op != GETF, E_ASSIGN_RHS, lhs);
            assert((int)lhs == OC(code));
            emit(top + 3, op + 1, OA(code), VAL_REG(OB(code)), expr(top + 2));
        }
    }
    }
    return isReturn;
}

static int topAbove(Value a, int top) {
    return IS_REG(a) ? max((int)a + 1, top) : top;
}

#define HERE (proto->code.size())

void Parser::forStat() {
    consume(TK_for);
    syms->enterBlock(false);    
    CERR(TOKEN != TK_NAME, E_FOR_NAME, VNIL);

    Value name = lexer->info.name;
    int slot = syms->localsTop();
    advance();
    consume(':'+TK_EQUAL);
    patchOrEmitMove(slot+2, slot+2, expr(slot+2));
    consume(':');
    patchOrEmitMove(slot+3, slot+1, expr(slot+3));
    int pos1 = emitHole();
    int pos2 = HERE;
    syms->set(name, slot);
    syms->addLocalsTop(2);
    insideBlock();
    emitJump(HERE, LOOP, VAL_REG(slot), pos2);
    emitJump(pos1, FOR,  VAL_REG(slot), HERE);
    syms->exitBlock(false);
}

void Parser::whileStat() {
    consume(TK_while);
    int pos1 = emitHole();
    int pos2 = HERE;
    Value a = expr(syms->localsTop());
    Vector<int> copyExp;
    Vector<int> &code = proto->code;
    while (code.size() > pos2) {
        copyExp.push(code.pop());
    }
    block();
    emitJump(pos1, JMP, UNUSED, HERE);
    while (copyExp.size()) {
        code.push(copyExp.pop());
    }
    emitJump(HERE, JT, a, pos2);
}

void Parser::ifStat() {
    consume(TK_if);
    Value a = expr(syms->localsTop());
    int pos1 = emitHole();
    block();
    if (lexer->token == TK_else) {
        int pos2 = emitHole();
        emitJump(pos1, JF, a, HERE);
        consume(TK_else);
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

static bool isUnaryOp(int token) {
    return token=='!' || token=='-' || token=='#' || token=='~';
}

int Parser::createUpval(Proto *proto, int protoLevel, Value name, int fromLevel, int slot) {
    // printf("up %d %s %d %d\n", protoLevel, GET_CSTR(name), fromLevel, slot);
    assert(fromLevel < protoLevel);
    if (fromLevel < protoLevel - 1) {
        slot = createUpval(proto->up, protoLevel - 1, name, fromLevel, slot);
    }    
    int protoSlot = proto->newUp(slot);
    syms->setUpval(name, protoSlot, protoLevel);
    return protoSlot;
}

int Parser::lookupSlot(Value name) {
    int slot, protoLevel;
    bool found = syms->get(name, &slot, &protoLevel, 0);
    CERR(!found, E_NAME_NOT_FOUND, name);

    const int curProtoLevel = syms->protoLevel();
    assert(protoLevel <= curProtoLevel);
    if (protoLevel < curProtoLevel) {
        slot = createUpval(proto, curProtoLevel, name, protoLevel, slot);
    }
    assert(slot < syms->localsTop());
    return slot;
}

Value Parser::mapExpr(int top) {
    consume('{');
    if (TOKEN=='}') {
        consume('}');
        return gc->EMPTY_MAP;
    }
    int slot = top;
    
    Map *map = Map::alloc(gc);
    Value mapValue = VAL_OBJ(map);
    for (int pos = 0; ; ++pos) {
        if (TOKEN == '}') { break; }
        Value k;
        if (TOKEN == TK_NAME && lexer->lookahead() == '=') {
            k = lexer->info.name;
            consume(TK_NAME);
            consume('=');
        } else {
            k = expr(top+1);
            consume(':');
        }
        Value v = expr(topAbove(k, top+1));
        
        if (IS_REG(k) || IS_REG(v)) {
            if (!IS_REG(mapValue)) {
                emit(slot, MOVE, slot, mapValue, UNUSED);
                mapValue = VAL_REG(slot);
            }
            emit(top+2, SETI, slot, k, v);
        } else {
            map->indexSet(k, v);
        }
        if (TOKEN == '}') { break; }
        consume(',');
    }    
    consume('}');
    return mapValue;
}

Value Parser::arrayExpr(int top) {
    consume('[');
    if (TOKEN==']') {
        consume(']');
        return gc->EMPTY_ARRAY;
    }
    int slot = top++;
    Array *array = Array::alloc(gc);
    Value arrayValue = VAL_OBJ(array);

    for (int pos = 0; ; ++pos) {
        if (TOKEN == ']') { break; }
        Value elem = expr(top);
        if (IS_REG(elem)) {
            if (!IS_REG(arrayValue)) {
                emit(slot, MOVE, slot, arrayValue, UNUSED);
                arrayValue = VAL_REG(slot);
            }
            emit(top+1, SETI, slot, VAL_NUM(pos), elem);
        } else {
            array->push(elem);
        }
        if (TOKEN == ']') { break; }
        consume(',');
    }
    consume(']');
    return arrayValue;
}

#define ARG_LIST(element) if (TOKEN != ')') { while (true) { element; if (TOKEN == ')') { break; } consume(','); } }

void Parser::parList() {
    consume('(');
    ++proto->nArgs;
    syms->set(String::value(gc, "this"));
    bool hasEllipsis = false;
    ARG_LIST(
             if (TOKEN == '*') {
                 advance();
                 hasEllipsis = true;
             }
             CERR(TOKEN != TK_NAME, E_SYNTAX, VNIL);
             ++proto->nArgs;
             syms->set(lexer->info.name);
             advance();
             );
    consume(')');
    if (hasEllipsis) {
        assert(proto->nArgs > 0);
        proto->nArgs = -proto->nArgs;
    }
}

Value Parser::callExpr(int top, Value a, Value self) {
    consume('(');
    int base = top;
    CERR(!IS_REG(a), E_CALL_NOT_FUNC, a);
    if (IS_REG(a) && (int)a == base) { ++base; }
    emit(base, MOVE, base, self, UNUSED);
    // patchOrEmitMove(base, base, self);
    int nArg = 1;
    int starPos = 0;
    ARG_LIST(
             if (TOKEN == '*') {                 
                 CERR(starPos, E_ELLIPSIS, VNIL);
                 advance();
                 starPos = nArg;
             }
             int argPos = base + nArg;
             patchOrEmitMove(argPos, argPos, expr(argPos));
             ++nArg;
             );
    consume(')');
    CERR(starPos && starPos != nArg - 1, E_ELLIPSIS, VNIL);
    if (starPos) {
        nArg = -nArg;
    }
    emit(0, CALL, base, a, VAL_NUM(nArg));
    if (base != top) {
        emit(base+1, MOVE, top, VAL_REG(base), UNUSED);
    }
    return VAL_REG(top);
}

Value Parser::suffixedExpr(int top) {
    Value a = VNIL;
    bool indexOnly = false;
    bool callOnly  = false;
    switch (lexer->token) {
    case TK_INTEGER: a = VAL_NUM(lexer->info.intVal); advance(); return a;
    case TK_DOUBLE:  a = VAL_NUM(lexer->info.doubleVal); advance(); return a;
    case TK_nil:     a = VNIL; advance(); return a;

    case TK_NAME: {
        Value name = lexer->info.name;
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
        a = lexer->info.name;
        advance();
        indexOnly = true;
        break;

    case '[': a = arrayExpr(top); indexOnly = true; break;
    case '{': a = mapExpr(top);   indexOnly = true; break;

    case TK_fn: a = funcExpr(top); callOnly = true; break;

    default: CERR(true, E_SYNTAX, VNIL);
    }

    while (true) {
        int t = TOKEN;
        if ((indexOnly && t != '[' && t !='.') || (callOnly && t != '(')) {
            return a;
        }
        indexOnly = callOnly = false;
        switch(t) {
        case '[': {
            advance();
            Value b = expr(top+1);
            if (TOKEN == ':') {
                advance();
                patchOrEmitMove(top+2, top+1, b);
                Value c = expr(top+2);
                patchOrEmitMove(top+3, top+2, c);
                emit(top+3, GETS, top, a, VAL_REG(top+1));
            } else {
                emit(top+2, GETI, top, a, b);
            }
            a = VAL_REG(top++);
            consume(']');
            break;
        }

        case '.':
            advance();
            CERR(TOKEN != TK_NAME, E_SYNTAX, VNIL);
            if (IS_REG(a) && (int)a == top) {
                ++top;
            }
            emit(top, GETF, top, a, lexer->info.name);
            advance();
            a = TOKEN == '(' ? callExpr(top+1, VAL_REG(top), a) : VAL_REG(top);
            ++top;
            break;

        case '(': a = callExpr(top++, a, VNIL); break;

        default: return a;
        }
    }
}

Proto *Parser::parseProto(int *outSlot) {
    consume(TK_fn);
    int slot = -1;
    if (TOKEN == TK_NAME) {
        slot = syms->set(lexer->info.name);
        advance();
    }
    proto = Proto::alloc(gc, proto);
    syms->enterBlock(true);
    parList();
    bool hasReturn = TOKEN == '=' ? lambdaBody() : insideBlock();
    if (!hasReturn) {
        emit(0, RET, 0, VNIL, UNUSED);
    }
    proto->freeze();
    Proto *funcProto = proto;
    proto->maxLocalsTop = syms->exitBlock(true);
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
    return VNIL;
}

static Value foldBinary(GC *gc, int op, Value a, Value b) {
    if (!IS_REG(a) && !IS_REG(b)) {
        switch (op) {
        case '+': return doAdd(gc, a, b);
        case '-': return doSub(a, b);
        case '*': return doMul(a, b);
        case '/': return doDiv(a, b);
        case '%': return doMod(a, b);
        case '^': return doPow(a, b);            
        }
    }
    return VNIL;
}

Value Parser::codeUnary(int top, int op, Value a) {
    {
        Value c = foldUnary(op, a);
        if (!IS_NIL(c)) { return c; }
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
        Value c = foldBinary(gc, op, a, b);
        if (!IS_NIL(c)) { return c; }
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

    case TK_xor: opcode = XOR; break;
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

    case TK_and:
        break;

    case TK_or:
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
    case TK_xor:
    case '|': return 6;

    case '='+TK_EQUAL: case '!'+TK_EQUAL:
    case '<': case '<'+TK_EQUAL: 
    case '>': case '>'+TK_EQUAL:
    case TK_IS: case TK_NOT_IS:
        return 5;

    case TK_and: return 4;
    case TK_or:  return 2;

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
        const int rightPrio = op == '^' || op == TK_and || op == TK_or ? prio-1 : prio;
        if (op == TK_and || op == TK_or) {
            if (!IS_REG(a)) {
                int saveCodeSize = proto->code.size();
                Value b = subExpr(top, rightPrio);
                if ((op==TK_and && IS_FALSE(a)) ||
                    (op==TK_or  && !IS_FALSE(a))) {
                    // drop b
                    proto->code.setSize(saveCodeSize);
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
                if (op == TK_and) {
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

Value Parser::expr(int top) {
    Value a = subExpr(top, 0);
    if (TOKEN != '?') { 
        return a; 
    }
    advance();
    int pos1 = emitHole();
    Value b = expr(top);
    patchOrEmitMove(top, top, b);
    int pos2 = emitHole();
    emitJump(pos1, JF, a, HERE);
    consume(':');
    Value c = expr(top);
    patchOrEmitMove(top, top, c);
    emitJump(pos2, JMP, UNUSED, HERE);
    return VAL_REG(top);
}

// code generation below

void Parser::patchOrEmitMove(int top, int dest, Value src) {
    int patchPos = proto->patchPos;
    assert(patchPos == -1 || patchPos == (int)proto->code.size()-1 || patchPos == (int)proto->code.size()-3);
    if (dest >= 0 && IS_REG(src) && (int)src >= 0 && patchPos >= 0) {
        int srcSlot = (int) src;
        unsigned code = proto->code.get(patchPos);
        int op = OP(code);
        if (opcodeHasDest(op)) {
            int oldDest = OC(code);
            assert(srcSlot == oldDest);        
            if (oldDest == dest) { return; } // everything is in the right place
            proto->code.setDirect(patchPos, CODE_CAB(OP(code), dest, OA(code), OB(code)));
            return; // patched
        }
    }
    emit(top, MOVE, dest, src, UNUSED);
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

Value Parser::mapSpecialConsts(Value a) {
    if (IS_NIL(a)) {
        return VAL_REG(UP_NIL);
    } else if (a == ZERO) {
        return VAL_REG(UP_ZERO);
    } else if (a == ONE) {
        return VAL_REG(UP_ONE);
    } else if (a == VAL_NUM(-1)) {
        return VAL_REG(UP_NEG_ONE);
    } else if (a == EMPTY_STRING) {
        return VAL_REG(UP_EMPTY_STRING);
    } else if (a == gc->EMPTY_ARRAY) {
        return VAL_REG(UP_EMPTY_ARRAY);
    } else if (a == gc->EMPTY_MAP) {
        return VAL_REG(UP_EMPTY_MAP);
    }
    return a;
}

void Parser::emitJump(int pos, int op, Value a, int to) {
    assert(to  >= 0  && to <= proto->code.size());
    assert(pos >= 0 && pos <= proto->code.size());
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
    proto->code.setExtend(pos, CODE_CD(op, a, offset));
}

void Parser::emit(unsigned top, int op, int dest, Value a, Value b) {
    if (a == gc->EMPTY_ARRAY) {
        emitCode(top, MOVE_V, top, VAL_REG(CONST_EMPTY_ARRAY), UNUSED);
        a = VAL_REG(top);
        ++top;
    } else if (a == gc->EMPTY_MAP) {
        emitCode(top, MOVE_V, top, VAL_REG(CONST_EMPTY_MAP),   UNUSED);
        a = VAL_REG(top);
        ++top;
    }
    if (b == gc->EMPTY_ARRAY) {
        emitCode(top, MOVE_V, top, VAL_REG(CONST_EMPTY_ARRAY), UNUSED);
        b = VAL_REG(top);
        ++top;
    } else if (b == gc->EMPTY_MAP) {
        emitCode(top, MOVE_V, top, VAL_REG(CONST_EMPTY_MAP),   UNUSED);
        b = VAL_REG(top);
        ++top;
    }
    
    // if (op == MOVE && isShortInt(a) && 
    
    a = mapSpecialConsts(a);
    if (op != CALL && op != SHL && op != SHR) {
        b = mapSpecialConsts(b);
    }
    emitCode(top, op, dest, a, b);
}

void Parser::emitCode(unsigned top, int op, int dest, Value a, Value b) {
    // assert(dest >= 0);
    proto->patchPos = proto->code.size();

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
            if (IS_OBJ(a)) {
                proto->consts.push(GET_OBJ(a));
            }
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
        proto->patchPos = proto->code.size();
        proto->code.push(CODE_CAB(op+1, dest, a, (int) GET_NUM(b)));
        return;
    }
    if (!IS_REG(b)) {
        emitCode(top+1, MOVE, top, b, UNUSED);
        b = VAL_REG(top);
    }
    proto->patchPos = proto->code.size();
    proto->code.push(CODE_CAB(op, dest, a, b));
}

int Parser::emitHole() {
    proto->patchPos = -1;
    return (int) proto->code.push(0);
}
