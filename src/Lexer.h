#pragma once

#include "SymbolMap.h"
#include "common.h"

enum TOKENS {
    TK_BEGIN_KEYWORD = 1,
    TK_BREAK = 1,
    TK_CONTINUE,
    TK_ELSE,
    TK_FOR,
    TK_FUNC,
    TK_GOTO,
    TK_IF,
    TK_NIL,
    TK_RETURN,
    TK_VAR,
    TK_END_KEYWORD,

    TK_INTEGER, TK_DOUBLE, TK_NAME, TK_STRING, TK_END,
    TK_EQUAL = 257,
};

union TokenInfo {
    s64 intVal;
    double doubleVal;
    struct {
        const char *strVal;
        int strLen;
    };
    u64 nameHash;
};

class Lexer {
    const char *string, *p, *end;
    int lineNumber;
    char *readString();
    // int error, errorPos, errorExpected;
    SymbolMap keywords;

    char *readString(int *outLen);

    int advanceInt(TokenInfo *info);

 public:
    Lexer(const char *string);

    int advance();
    int lookahead();

    int token;
    TokenInfo info;
};
