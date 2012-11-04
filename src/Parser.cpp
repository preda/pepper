#include "Parser.h"
#include "Lexer.h"
#include "VM.h"

#define ANYWHERE (-1)

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

static byte getRegValue(Value a) {
    s64 v;
    int ta = TAG(a);
    return
        ta==REGISTER && (int)a >= 0 ? (int) a :
        ta==REGISTER ? 0x80 | (-(int)a) :
        ta==INTEGER && (v=getInteger(a))>=-64 && v<64) ? (unsigned)a & 0x7f :
        ta==ARRAY || ta==MAP || ta==STRING ? 0xf0 | ta :
        ta==OBJECT ? 0xff : 0xfe;
}

Value Parser::code(int op, Value a, Value b) {
    int dest = top++;
    bool aIsReg = IS_REGISTER(a) && (int)a >= 0;
    byte ra = getRegValue(a);
    
    bool bIsReg = IS_REGISTER(b) && (int)b >= 0;
    byte rb = getRegValue(b);

    bytecode.push(CODE(op, dest, ra, rb));
    if (!aIsReg && ra==0xfe) {
        bytecode.push((unsigned)a);
        bytecode.push((unsigned)(a>>32));
    }
    if (!bIsReg && rb==0xfe) {
        bytecode.push((unsigned)b);
        bytecode.push((unsigned)(b>>32));
    }
    return VAL_REG(dest);
}

#define REG0 VAL_REG(0)
#define TRUE  VAL_INT(1)
#define FALSE VAL_INT(0)

Value Parser::codeUnary(int op, Value a) {
    switch (op) {
    case '!': return 
        a==NIL ? TRUE :
        IS_NUMBER(a) ? (getDouble(a)==0 ? TRUE : FALSE) :
        IS_REGISTER(a) ? code(NOT, a, REG0) : FALSE;
             
    case '-': return
        IS_INTEGER(a) ? VAL_INT(-getInteger(a)) :
        IS_DOUBLE(a)  ? VAL_DOUBLE(-getDouble(a)) :
        IS_REGISTER(a) ? code(SUB, VAL_INT(0), a) : ERR;

    case '~': return
        IS_INTEGER(a) ? VAL_INT(~getInteger(a)) :
        IS_REGISTER(a) ? code(XOR, a, VAL_INT(-1)) : ERR;
        
    case '#': return
        IS_ARRAY(a)  ? Array::len(a) :
        IS_STRING(a) ? String::len(a) :
        IS_MAP(a)    ? Map::len(a) :
        IS_REGISTER(a) ? code(LEN, a, REG0) : ERR;
    }
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
        op == '^' ? POW;
    return code(opcode, a, b);
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

SymbolData Parser::createUpval(u64 name, SymbolData sym, int level) {    
    if (sym.level < level - 1) {
        sym = createUpval(name, sym, level - 1);
    }
    assert(sym.level == level - 1);
    proto->ups.push(sym.slot);
    return symbolTable.set(level, name, KIND_REGUP, proto.ups.size - 1);
}

SymbolData Parser::lookupName(u64 name) {
    SymbolData s = symbolTable.get(name);
    if (s.kind != KIND_EMPTY) {
        assert(s.level <= curLevel);
        if (s.level < curLevel) {
            s = createUpval(name, s, level);
        }
    }
    return s;
}

Value Parser::primaryExp() {
    switch (token) {
    case '(': {
        advanceToken();
        Value a = expr();
        consume(')');
        return a;
    }
        
    case TK_NAME: {
        u64 name = hash64(tokenInfo.strVal, tokenInfo.strLen);
        consume(TK_NAME);
        SymbolData sym = symbolTable.get(name);
        if (sym.kind == KIND_EMPTY) {
            ERR(); // symbol not found
            return ERR;
        }
        
        if (sym.level == curLevel) {
            return VAL_REG(sym.slot);
        } else {
            // create new upval in current level
            createUpval(name, sym, curLevel);
        }
    }
    }
}

        /*
            if (sym.kind == KIND_LOCAL) {
                return VAL_REG(sym.slot);
            } else if (sym.kind == KIND_UPVAL) {
                return VAL_REG(-sym.slot);
            } else {
                return ERR;
                }*/


Value Parser::suffixedExpr() {
    Value a = primaryExpr();
    while (true) {
        switch (token) {
        case '(':
            funcargs();
            break;

        default:
            
        }
    }
}

Value Parser::simpleExpr() {
    switch (token) {
    case TK_INTEGER: return VAL_INT(tokenInfo.intVal);
    case TK_DOUBLE:  return VAL_DOUBLE(tokenInfo.doubleVal);
    case TK_STRING:  return VAL_STRING(tokenInfo.strVal, tokenInfo.strLen);
    case TK_NIL:     return NIL;

    case '[': // array TODO
        break;

    case '{': // map TODO
        break;

    case TK_FUNC:
        break;

    default:
        return suffixedExp();
    }
}

Value Parser::subExpr(int limit) {
    Value a;
    if (isUnaryOp(token)) {
        int op = token;
        advanceToken();
        a = codeUnary(op, subExpr(8));
    } else {
        a = simpleExpr(dest);
    }
    while (binaryPriorityLeft(token) > limit) {
        int op = token;
        advanceToken();
        a = codeBinary(op, a, subExpr(binaryPriorityRight(op)));
    }
    return a;
}

Value Parser::expr() {
    return subExpr(0);
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
