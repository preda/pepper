#pragma once

#include "SymbolMap.h"
#include "Vector.h"
#include "common.h"

enum TOKENS {
    TK_BEGIN_KEYWORD = 1,
    TK_BREAK = 1,
    TK_CONTINUE,
    TK_ELSE,
    TK_FOR,
    TK_WHILE,
    TK_FUNC,
    TK_GOTO,
    TK_IF,
    TK_NIL,
    TK_RETURN,
    TK_VAR,

    TK_LOG_AND,
    TK_LOG_OR,
    TK_BIT_XOR,

    TK_END_KEYWORD,

    TK_SHIFT_L, TK_SHIFT_R,
    TK_IS, TK_NOT_IS,
   
    TK_INTEGER, TK_DOUBLE, TK_NAME, TK_STRING, TK_END,
    TK_EQUAL = 257,
};

struct TokenInfo {
    union {
        s64 intVal;
        double doubleVal;
        Value stringVal;
        u64 nameHash;
    };
    Vector<char> name;
};

class Lexer {
    const char *string, *end;


    SymbolMap keywords;
    Value readString();
    int advanceInt(TokenInfo *info);

 public:
    const char *p;
    int lineNumber;

    Lexer(const char *string);

    int advance();
    int lookahead();

    int token;
    TokenInfo info;
};
