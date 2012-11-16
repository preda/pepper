#include "Parser.h"
#include "Proto.h"
#include "SymbolTable.h"
#include "Decompile.h"

#include <stdio.h>

const char *test[] = {
    // "var a = 1\n var b=2 var c = a+b",
    // "var foo = 500 var boo = \"roo\"",
    // "var cond = 1 if cond { cond = cond + 1 } var b = 3", 
    "var a = 1 var b if b[a] { b[a] = 2 }",
    0,
};

void compileDecompile(const char *text) {
    Proto proto;
    SymbolTable syms;
    if (Parser::parseStatList(&proto, &syms, text)==0) {
        printf("\n\n");
        printProto(&proto);
    }    
}

int main(int argc, char **argv) {    
    if (argc > 1) {
        compileDecompile(argv[1]);
    } else {
        for (const char **p = test; *p != 0; ++p) {
            compileDecompile(*p);
        }
    }
    return 0;
}
