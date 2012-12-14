#include "Parser.h"
#include "common.h"
#include <setjmp.h>
#include <stdio.h>

__thread jmp_buf jumpBuf;

static const char *errorMes[32] = {
#define _(x) #x
#include "error.inc"
#undef _
};

u64 error(const char *file, int line, int err) {
    if (err >= E_EXPECTED) {
        int token = err - E_EXPECTED;
        fprintf(stderr, "ERROR expected token %d '%c' at %s:%d\n", token, (char)(token>=32?token:' '), file, line);
    } else {
        fprintf(stderr, "ERROR %d %s at %s:%d\n", err, errorMes[err], file, line);
    }
    __builtin_abort();
    longjmp(jumpBuf, err); 
}
