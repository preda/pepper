#include "Parser.h"
#include "Lexer.h"
#include "StringBuilder.h"
#include "common.h"
#include <setjmp.h>
#include <stdio.h>

__thread jmp_buf jumpBuf;

static const char *errorMes[32] = {
#define _(x) #x
#include "error.inc"
#undef _
};

const char *tokenToString(char *buf, int size, int token) {
    const char *tail = token >= TK_EQUAL ? "=" : "";
    if (token >= TK_EQUAL) { token -= TK_EQUAL; }
    if (token >= 32) {
        snprintf(buf, size, "'%c'%s", (char)token, tail);
    } else {
        snprintf(buf, size, "%s%s", tokenStrings[token], tail);
    }
    return buf;
}

u64 error(const char *file, int line, int err, Value val, Lexer *lexer) {
    if (err >= E_EXPECTED) {
        int token = err - E_EXPECTED;
        char buf[32];        
        fprintf(stderr, "ERROR expected token %s (#%d) at %s:%d\n",
                tokenToString(buf, sizeof(buf), token), token, file, line);
    } else {
        fprintf(stderr, "ERROR %d %s at %s:%d\n", err, errorMes[err], file, line);
    }
    if (!IS_NIL(val)) {
        StringBuilder builder;
        builder.append(val);
        fprintf(stderr, "%s\n", builder.cstr());
    }
    if (lexer) {
        lexer->printLocation();
    }
    __builtin_abort();
    longjmp(jumpBuf, err); 
}
