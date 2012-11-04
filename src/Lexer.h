#pragma once

#include "SymbolMap.h"

static const char *tokens[] = {
    "<begin-keyword>",
    "break", "continue", 
    "else", "for", "fun", "goto",
    "if", "nil", "return", "var",
    "<end-keyword>", "<number>", "<name>", "<string>", "<eos>", "<error>",
};

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

    TK_INTEGER, TK_DOUBLE, TK_NAME, TK_STRING, TK_EOS, TK_ERROR,
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
    char *string, *p, *end;
    int lineNumber;
    char *readString();
    int error, errorPos, errorExpected;
    SymbolMap keywords;

 public:
    Lexer(char *string);

    int nextToken(TokenInfo *info);

    void setError(int expected) {
        error = 1;
        errorExpected = expected;
    }
};
