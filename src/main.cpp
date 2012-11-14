#include "Parser.h"
#include "Proto.h"
#include "SymbolTable.h"

#include <stdio.h>

int main(int argc, char **argv) {
    Proto proto;
    SymbolTable syms;
    const char *text="var a = 1\n\
var b=2 var c = a+b";
    Parser::parseStatList(&proto, &syms, text);
    printf("\n\n");
    proto.print();
    return 0;
}
