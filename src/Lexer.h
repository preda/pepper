#pragma once

#include "Vector.h"
#include "common.h"

enum TOKENS {
#define _(tok) TK_##tok
#include "tokens.inc"
#undef _
    TK_EQUAL = 257,
};

extern const char *tokenStrings[];

struct TokenInfo {
    union {
        s64 intVal;
        double doubleVal;
        Value name;
    };
};

class GC;
class Map;

class Lexer {
    const char *string, *end;
    GC *gc;
    Map *keywords;
    Vector<char> name;

    Value readString(char endChar);
    int advanceInt(TokenInfo *info);

 public:
    const char *p;
    const char *pLine;
    int lineNumber;

    Lexer(GC *gc, const char *string);

    int advance();
    int lookahead();

    int token;
    TokenInfo info;
};
