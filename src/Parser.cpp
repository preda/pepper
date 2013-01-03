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
#include "Pepper.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <setjmp.h>

#define UNUSED VAL_REG(0)
#define TOKEN (lexer->token)

Parser::Parser(Pepper *context, Proto *proto, SymbolTable *syms, Lexer *lexer) {
    this->proto = proto;
    this->syms  = syms;
    this->lexer = lexer;
    this->context = context;
    this->gc = context->getGC();
    EMPTY_ARRAY = VAL_OBJ(Array::alloc(gc));
    EMPTY_MAP   = VAL_OBJ(Map::alloc(gc));
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

Func *Parser::parseFunc(Pepper *context, SymbolTable *syms, Value *upsTop, const char *text) {
    GC *gc = context->getGC();
    Lexer lexer(gc, text);

    if (int err = setjmp(jumpBuf)) {
        printf("err %d at line %d, before '%s'\n", err, lexer.lineNumber, lexer.p);
        return 0;
    }

    Proto *proto = Proto::alloc(gc, 0);
    Parser parser(context, proto, syms, &lexer);
    int slot;
    Proto *funcProto = parser.parseProto(&slot);
    return makeFunc(gc, funcProto, upsTop, slot);
}

Func *Parser::parseStatList(Pepper *context, SymbolTable *syms, Value *upsTop, const char *text) {
    GC *gc = context->getGC();
    Proto *proto = Proto::alloc(gc, 0);
    if (Parser::parseStatList(context, proto, syms, text)) { return 0; }
    close(proto);
    return makeFunc(gc, proto, upsTop, -1);
}

Func *Parser::makeFunc(GC *gc, Proto *proto, Value *upsTop, int slot) {
    Value dummyRegs;
    return Func::alloc(gc, proto, upsTop, &dummyRegs, slot);
}

int Parser::parseStatList(Pepper *context, Proto *proto, SymbolTable *syms, const char *text) {
    Lexer lexer(context->getGC(), text);
    if (int err = setjmp(jumpBuf)) {
        printf("at line %d, '%s'\n", lexer.lineNumber, lexer.p);
        return err;
    }
    Parser parser(context, proto, syms, &lexer);
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
            int slot = lookupSlot(lexer->info.name);
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
        emit(proto->localsTop, RET, 0, TOKEN == ';' ? VNIL : expr(proto->localsTop), UNUSED);
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
        int op = OP(code);
        ERR(op != GETI && op != GETF, E_ASSIGN_RHS); // (lhs & FLAG_DONT_PATCH)
        assert((int)lhs == OC(code));
        emit(proto->localsTop+3, op+1, OA(code), VAL_REG(OB(code)), expr(proto->localsTop+2));
    }    
}

#define HERE (proto->code.size())

void Parser::forStat() {
    consume(TK_FOR);
    ERR(TOKEN != TK_NAME, E_FOR_NAME);
    Value name = lexer->info.name;
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
    proto->localsTop++;
    block();
    //TODO assert proto->localsTop reverted
    emitJump(HERE, LOOP, VAL_REG(slot), pos2);
    emitJump(pos1, FOR,  VAL_REG(slot), HERE);
}

void Parser::whileStat() {
    consume(TK_WHILE);
    unsigned pos1 = emitHole();
    unsigned pos2 = HERE;
    Value a = expr(proto->localsTop);
    Vector<unsigned> copyExp;
    Vector<unsigned> &code = proto->code;
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
    Value name = lexer->info.name;
    advance();
    int top = proto->localsTop;
    Value a = VNIL;
    int aSlot = -1;
    if (TOKEN == '=' || TOKEN == ':'+TK_EQUAL) {
        advance();
        a = expr(top);
        Value s = syms->get(name);
        int level = s & 0xff;
        int slot  = s >> 8;
        if (!IS_NIL(s) && level == proto->level && slot >= 0) {
            aSlot = slot; // reuse existing local with same name
        }
    } else {
        Value s = lookupName(name);
        if (!IS_NIL(s)) {
            int slot = s >> 8;
            if (slot < 0) {
                a = VAL_REG(slot); // init from upval with same name
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

int Parser::createUpval(Proto *proto, Value name, int level, int slot) {
    assert(level < proto->level);
    if (level < proto->level - 1) {
        slot = createUpval(proto->up, name, level, slot);
    }
    // assert(level == proto->level - 1);
    proto->addUp(slot);
    // return syms->set(proto->level, name, -proto->nUp());
    return -proto->nUp();
}

int Parser::lookupName(Value name) {
    Value s = syms->get(name);
    if (IS_NIL(s)) {
        return 256; //marker not found
    }
    int v = (int) s;
    int level = v & 0xff;
    int slot  = v >> 8;
    assert(level <= proto->level);
    if (level < proto->level) {
        slot = createUpval(proto, name, level, slot);
    }
    assert(slot < proto->localsTop);
    return slot;
    
    /*
    if (s.kind != KIND_EMPTY) {
        assert(s.level <= proto->level);
        if (s.level < proto->level) {
            s = createUpval(proto, name, s);
        }
    }
    assert(s.slot < proto->localsTop);
    return s;
    */
}

int Parser::lookupSlot(Value name) {
    int slot = lookupName(name);
    ERR(slot == 256, E_NAME_NOT_FOUND);
    return slot;
}

Value Parser::mapExpr(int top) {
    consume('{');
    if (TOKEN=='}') {
        consume('}');
        return EMPTY_MAP;
    }
    int slot = top;
    
    Map *map = Map::alloc(gc);
    emit(slot, ADD, slot, EMPTY_MAP, VAL_OBJ(map));

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
            emit(top+2, SETI, slot, k, v);
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
    Array *array = Array::alloc(gc);
    emit(slot, ADD, slot, EMPTY_ARRAY, VAL_OBJ(array));

    for (int pos = 0; ; ++pos) {
        if (TOKEN == ']') { break; }
        Value elem = expr(top);
        if (IS_REG(elem)) {
            emit(slot, SETF, slot, VAL_NUM(pos), elem);
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
    ++proto->nArgs;
    syms->set(String::value(gc, "this"), proto->localsTop++);
    bool hasEllipsis = false;
    ARG_LIST(
             if (TOKEN == '*') {
                 advance();
                 hasEllipsis = true;
             }
             ERR(TOKEN != TK_NAME, E_SYNTAX);
             ++proto->nArgs;
             syms->set(lexer->info.name, proto->localsTop++);
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
    ERR(!IS_REG(a), E_CALL_NOT_FUNC);
    if (IS_REG(a) && (int)a == base) { ++base; }
    emit(base, MOVE, base, self, UNUSED);
    // patchOrEmitMove(base, base, self);
    int nArg = 1;
    int starPos = 0;
    ARG_LIST(
             if (TOKEN == '*') {                 
                 ERR(starPos, E_ELLIPSIS);
                 advance();
                 starPos = nArg;
             }
             int argPos = base + nArg;
             patchOrEmitMove(argPos, argPos, expr(argPos));
             ++nArg;
             );
    consume(')');
    ERR(starPos && starPos != nArg - 1, E_ELLIPSIS);
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
    case TK_NIL:     a = VNIL; advance(); return a;

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

    case TK_FUNC: a = funcExpr(top); callOnly = true; break;

    default: ERR(true, E_SYNTAX);
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
            ERR(TOKEN != TK_NAME, E_SYNTAX);
            if (IS_REG(a) && (int)a == top) {
                ++top;
            }
            emit(top+1, GETF, top, a, lexer->info.name);
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
    consume(TK_FUNC);
    int slot = -1;
    if (TOKEN == TK_NAME) {
        syms->set(lexer->info.name, slot=proto->localsTop++);
        advance();
    }
        /*
        Value s = syms->get(name);
        int level = s & 0xff;
        int sSlot  = s >> 8;
        if (!IS_NIL(s) && level == proto->level && sSlot >= 0) {
            slot = sSlot;
        } else {
            slot = proto->localsTop++;
            syms->set(name, slot);
        }
        */

    proto = Proto::alloc(gc, proto);
    syms->pushContext();
    parList();
    block();
    emit(0, RET, 0, VNIL, UNUSED);
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
                int saveCodeSize = proto->code.size();
                Value b = subExpr(top, rightPrio);
                if ((op==TK_LOG_AND && IS_FALSE(a)) ||
                    (op==TK_LOG_OR  && !IS_FALSE(a))) {
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
    assert(patchPos == -1 || patchPos == (int)proto->code.size()-1 || patchPos == (int)proto->code.size()-3);
    if (dest >= 0 && IS_REG(src) && (int)src >= 0 && patchPos >= 0) {
        int srcSlot = (int) src;
        unsigned code = proto->code.get(patchPos);
        int op = OP(code);
        if (opcodeHasDest(op)) {
            int oldDest = OC(code);
            assert(srcSlot == oldDest);        
            if (oldDest == dest) { return; } // everything is in the right place
            proto->code.set(patchPos, CODE_CAB(OP(code), dest, OA(code), OB(code)));
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
    } else if (a == EMPTY_ARRAY) {
        return VAL_REG(UP_EMPTY_ARRAY);
    } else if (a == EMPTY_MAP) {
        return VAL_REG(UP_EMPTY_MAP);
    }
    return a;
}

void Parser::emitJump(unsigned pos, int op, Value a, unsigned to) {
    assert(to  <= proto->code.size());
    assert(pos <= proto->code.size());
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
    
    if ((op == SETI || op == SETF) && (dest == UP_EMPTY_ARRAY || dest == UP_EMPTY_MAP)) {
        assert(op == SETI);
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
