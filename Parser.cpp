#include "Parser.h"
#include "Lexer.h"
#include "VM.h"

Parser::Parser(Lexer *lexer) {
    this->lexer = lexer;
    top = 0;
    symbolTable.set("import", top++);

    token = lexer->nextToken(&tokenInfo);
}

void Parser::advanceToken() {
    token = lexer->nextToken(&tokenInfo);
}

void Parser::consume(int t) {
    if (t == token) {
        advanceToken();
    } else if (token != TK_ERROR) {
        lexer->setError(t);
    }
}

void Parser::var() {
    consume(TK_VAR);
    if (token == TK_NAME) {
        u64 name = hash64(tokenInfo.string);
        consume(TK_NAME);
        consume('=');
        int varReg = top++;
        expr(varReg);
        symbolTable.set(key, KIND_LOCAL, varReg);
    } else {
        consume(TK_NAME);
    }
}

static bool isUnaryOp(int token) {
    return token=='!' || token=='-' || token=='#' || token=='~';
}

static Value foldUnary(int op, Value a) {
    if (a != NIL) {
        switch (op) {
        case '!': return (IS_NUMBER(a) && getDouble(a)==0) ? VAL_INT(1) : VAL_INT(0);
        case '-': return doSub(VAL_INT(0), a);
        case '~': return doXor(VAL_INT(0), a);
            // return IS_INTEGER(a) ? VAL_INT(~ getInteger(a)) : NIL;
        case '#': return IS_ARRAY(a) ? Array::len(a) : NIL;
        }
    }
    return NIL;
}

static Value foldBinary(int op, Value a, Value b) {
    if (a != NIL && b != NIL) {
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

static int binaryPriorityLeft(int token) {
    switch (token) {
    case '+': case '-': return 6;
    case '*': case '/': case '%': return 7;
    case '^': return 10;
    case '='+TK_EQUAL: case '<': case '<'+TK_EQUAL: case '!'+TK_EQUAL:
    case '>': case '>'+TK_EQUAL: return 3;
    case '&': return 2;
    case '|': return 1;
    default : return 0;
    }
}

static int binaryPriorityRight(int token) {
    int left = binaryPriorityLeft(token);
    return token == '^' ? left-1 : left;
}

Value Parser::simpleExp(int dest) {
    switch (token) {
    case TK_INTEGER: return VAL_INT(tokenInfo.intVal);
    case TK_DOUBLE:  return VAL_DOUBLE(tokenInfo.doubleVal);
    case TK_STRING:
        
        break;

    case TK_NIL:
        break;

    case '{':
        break;

    case TK_FUNC:
        break;

    default:
        return suffixedExp(dest);
    }
}

Value Parser::subExpr(int dest, int limit) {
    Value a = NIL;
    if (isUnaryOp(token)) {
        int op = token;
        advanceToken();
        a = subexpr(dest, 8);
        Value c = foldUnary(op, a);
        if (c != NIL) {
            a = c;
            goto cont; 
        }
        if (a != NIL) {
            codeMoveRegVal(dest, a);
            a = NIL;
        }
        codeUnary(op, dest, dest);
    cont:
    } else {
        a = simpleExp(dest);
    }
    while (binaryPriorityLeft(token) > limit) {
        int op = token;
        advanceToken();
        Value b = subExpr(dest+1, binaryPriorityRight(op));
        Value c = foldBinary(op, a, b);
        if (c != NIL) {
            a = c;
            continue;
        }
        if (a != NIL) {
            codeMoveRegVal(dest, a);
            a = NIL;
        }
        if (b != NIL) {
            codeMoveRegVal(dest+1, b);
        }
        codeBinary(op, dest, dest, dest+1);
    }
}

Value Parser::expr(int destReg) {
    return subExpr(destReg, 0);
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
    while (token != '}' && token != TK_ERROR) {
        statement();
    }
}

void Parser::statement() {
    int reg = name();
    consume(':' + TK_EQUAL);
    simpleExp();

    /*
    switch (token) {
    case TK_IF: ifstat(); break;
    case TK_FOR: forstat(); break;
    default: exprstat(); break;
    }
    */
}

int Parser::name() {
    int reg = 0;
    if (token == TK_NAME) {
        const char *s = tokenInfo.string;
        Name *name = names.find(s);
        if (!name) {
            names.add(s, 1);
            name = names.buf + names.size-1;
        }
        reg = name->reg;
    }
    consume(TK_NAME);
    return reg;
}

/*
void Parser::ifstat() {    
}

void Parser::exprstat() {
}

void Parser::primaryexp() {
    switch (token) {
    case '(':
        advance();
        expr()
    }
}
*/
