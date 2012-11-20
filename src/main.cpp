#include "Parser.h"
#include "Proto.h"
#include "SymbolTable.h"
#include "Decompile.h"
#include "VM.h"

#include <stdio.h>
#include <assert.h>

const char *test[] = {
    /*
    "var a = 1\n var b=2 var c = a+b",
    "var foo = 500 var boo = \"roo\"",
    "var cond = 1 if cond { cond = cond + 1 } var b = 3", 
    "var b if b[1] { b[2] = 3 }",
    "\"foo\"[3][\"bar\"] = 1",
    "[\"foo\"] + [1, 2, 3]",
    "var a var b = [1, a]",
    "var a = func(x) { return x + x; }",
    "func() { return; }",
    "var a = func(x) { x = 1 } a(a+2, 2, a)",
    */
    "var a = 5; return a * 2",
    0,
};

Value eval(const char *text) {
    Func *f = Parser::parseStatList(text);
    return f ? VM().run(f) : NIL;
}

bool compileDecompile(const char *text) {
    printf("\n\"%s\"\n\n", text);
    Func *f = Parser::parseStatList(text);
    if (f) { 
        printFunc(f); 
        Value ret = VM().run(f);
        printValue(ret);
        return true;
    }
    return false;
}

int main(int argc, char **argv) {
    if (argc > 1) {
        compileDecompile(argv[1]);
    } else {
        for (const char **p = test; *p != 0; ++p) {
            if (!compileDecompile(*p)) { break; }
        }
    }
    return 0;
}

    /*
    Proto *proto = Proto::alloc(0);
    SymbolTable syms;
    int err = Parser::parseStatList(proto, &syms, text);
    if (!err) {
        Parser::close(proto);
        printf("\n\n%p\n", proto);
        printProto(proto);
        Func *f = Func::alloc(proto, 0, 0);
    }
    return err;
    */
