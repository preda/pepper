#include "Parser.h"
#include "Proto.h"
#include "SymbolTable.h"
#include "Decompile.h"
#include "VM.h"
#include "String.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

struct T {
    T(const char *s, Value v) : source(s), result(v) {}

    const char *source;
    Value result;
};

T tests[] = {
    T("return 13", VAL_INT(13)),
    T("var a = 10; var b=-2 var c = a+ b; return c", VAL_INT(8)),

    // strings
    T("var a = \"foo\" var b = \"bar\" return a + b", String::makeVal("foobar")),

    // array
    T("var a = [3, 4, 5]; return a[2]", VAL_INT(5)),
    T("var foobar = [3, 4, 5] return [3, 4, 5] == foobar", TRUE),
    T("var tralala = [13, 14] tralala[3]=\"tralala\"; return tralala[3]", String::makeVal("tralala")),

    // comparison
    T("return \"foo\" < \"bar\"", FALSE),
    T("return -3 < -2", TRUE),    
};

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
    // 'var strlen = ffi("strlen", "int (const char *)"); return strlen("bar hello foo")'
    // 'var p = ffi("printf", "void (const char *, ...)"); var a=5; p("hello %d\n", a)'
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
        const char *s;
        if (argv[1][0] == '-') {
            int n = atoi(argv[2]);
            s = tests[n].source;
        } else {
            s = argv[1];
        }
        compileDecompile(s);
    } else {
        int n = sizeof(tests) / sizeof(tests[0]);
        int nFail = 0;
        for (int i = 0; i < n; ++i) {
            T &t = tests[i];
            Value ret = eval(t.source);
            if (!equals(ret, t.result)) {
                printf("\n%d: '%s'\n", i, t.source);
                printValue(ret);
                printValue(t.result);
                ++nFail;
            } else {
                printf(".");
            }
        }
        printf("\nPassed %d tests out of %d\n", (n-nFail), n);
    }
}
