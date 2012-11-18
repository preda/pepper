#include "Parser.h"
#include "Proto.h"
#include "SymbolTable.h"
#include "Decompile.h"

#include <stdio.h>
#include <assert.h>

const char *test[] = {
    "var a = 1\n var b=2 var c = a+b",
    "var foo = 500 var boo = \"roo\"",
    "var cond = 1 if cond { cond = cond + 1 } var b = 3", 
    "var b if b[1] { b[2] = 3 }",
    "\"foo\"[3][\"bar\"] = 1",
    "[\"foo\"] + [1, 2, 3]",
    "var a var b = [1, a]",
    0,
};

int compileDecompile(const char *text) {
    Proto proto;
    SymbolTable syms;
    int err = Parser::parseStatList(&proto, &syms, text);
    if (!err) {
        printf("\n\n");
        printProto(&proto);
    }
    return err;
}

int main(int argc, char **argv) {
    if (argc > 1) {
        compileDecompile(argv[1]);
    } else {
        for (const char **p = test; *p != 0; ++p) {
            if (compileDecompile(*p)!=0) {
                break;
            }
        }
    }
    return 0;
}
