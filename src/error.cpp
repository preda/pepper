#include "Parser.h"
#include "common.h"
#include <setjmp.h>
#include <stdio.h>

__thread jmp_buf jumpBuf;

static const char *errorMes[32];

static void init() {
#define _(A) errorMes[E_##A]=#A
    _(VAR_NAME);
    _(NAME_NOT_FOUND);
    _(TODO);
    _(WRONG_TYPE);
    _(DIV_ZERO);
    _(ASSIGN_TO_CONST);
    _(ASSIGN_RHS);
    _(SYNTAX);
#undef _
}

u64 error(const char *file, int line, int err) {
    init();
    if (err >= E_EXPECTED) {
        int token = err - E_EXPECTED;
        fprintf(stderr, "ERROR expected token %d '%c' at %s:%d\n", token, (char)(token>=32?token:' '), file, line);
    } else {
        fprintf(stderr, "ERROR %d %s at %s:%d\n", err, errorMes[err], file, line);
    }
    longjmp(jumpBuf, err); 
}

int Parser::parseStatList(Proto *proto, SymbolTable *syms, const char *text) {
    if (int err = setjmp(jumpBuf)) {
        return err;
    }
    _parseStatList(proto, syms, text);
    return 0;
}
