#include "Parser.h"
#include "Proto.h"
#include "SymbolTable.h"
#include "Decompile.h"

#include <stdio.h>

const char *test[] = {
    // "var a = 1\n var b=2 var c = a+b",
    // "var foo = 500 var boo = \"roo\"",
    "var cond = 1 if cond { cond = cond + 1 } var b = 3", 
    0,
};

void compileDecompile(const char *text) {
    Proto proto;
    SymbolTable syms;
    Parser::parseStatList(&proto, &syms, text);
    printf("\n\n");
    printProto(&proto);    
}

int main(int argc, char **argv) {    
    for (const char **p = test; *p != 0; ++p) {
        compileDecompile(*p);
    }
    if (argc > 1) {
        compileDecompile(argv[1]);
    }
    return 0;
}
