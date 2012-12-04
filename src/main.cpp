#include "Parser.h"
#include "Proto.h"
#include "SymbolTable.h"
#include "Decompile.h"
#include "VM.h"
#include "String.h"
#include "Array.h"
#include "Map.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

struct T {
    T(const char *s, Value v) : source(s), result(v) {}

    const char *source;
    Value result;
};

T tests[] = {
    T("return nil", NIL),
    T("return -1", VAL_NUM(-1)),
    T("return 0", VAL_NUM(0)),
    T("return \"\"", EMPTY_STRING),
    T("return []", VAL_OBJ(Array::alloc())),
    T("return {}", VAL_OBJ(Map::alloc())),

    T("f:=func() { return 1 } return f()", ONE),
    T("f:=func(x) { return -1 } return f(5)", VAL_NUM(-1)),
    T("f:=func(x,y) { if x { return y } else { return 0 } }; return 1+f(0, 2)", ONE),
    T("f:=func(x,y) { if x { return y } else { return 0 } }; return 1+f(1, -1)", ZERO),

    T("return 13", VAL_NUM(13)),
    T("var a = 10; var b=-2 var c = a+ b; return c", VAL_NUM(8)),
    T("a:=3; b:=a; return a", VAL_NUM(3)),
    T("a:=2; a=3; b:=a; return a", VAL_NUM(3)),

    // strings
    T("var a = \"foo\" var b = \"bar\" return a + b", String::makeVal("foobar")),

    // array
    T("var a = [3, 4, 5]; return a[2]", VAL_NUM(5)),
    T("var foobar = [3, 4, 5] return [3, 4, 5] == foobar", TRUE),
    T("var tralala = [13, 14] tralala[3]=\"tralala\"; return tralala[3]", String::makeVal("tralala")),

    // comparison
    T("return \"foo\" < \"bar\"", FALSE),
    T("return -3 < -2", TRUE),
    T("return \"aaaaaaaa\" < \"b\"", TRUE),
    T("return [] < [1] && [1] < [2]", TRUE),

    // bool ops
    T("var a=2 var b=3 var c=nil return a+1 && b+2 || c+3", VAL_NUM(5)),
    T("return 1 && nil && 13", NIL),
    T("return 13 || 14", VAL_NUM(13)),

    T("var a=[]; a[1]=2; var b=[]; return b[1]", NIL),
    
    // len
    T("return #\"foo\"==3", TRUE),
    T("var tralala=\"tralala\" return #tralala", VAL_NUM(7)),
    T("var s6=\"abcabc\" if s6[6]==nil and s6[5]==\"c\" { return 7 } else{return 8}", VAL_NUM(7)),

    // ffi
    T("var strlen=ffi(\"strlen\", \"int (char *)\", \"c\"); return strlen(\"bar hello foo\")", VAL_NUM(13)),
    T("return ffi(\"strstr\", \"offset (char*, char *)\", \"c\")(\"big oneedle hayst\", \"need\")", VAL_NUM(5)),

    // assign
    T("a:=3; return a", VAL_NUM(3)),
    T("var a=2.5 b:=2*a return b+1", VAL_NUM(6)),

    // while
    T("i:=0; s:=0 while i < 4 { i = i + 1; s = s + i + 1 } return s", VAL_NUM(14)),
    T("i:=0 s:=0 while i < 1000000 { s=s+i+1 i=i+1 } return s", VAL_NUM(500000500000ll)),
    T("i:=0 s:=0 while i < 100000 { s=s+i+0.5 i=i+0.5 } return s", VAL_NUM(10000050000)),

    T("s:=0 for i:=0:5 {s = s + i}; return s", VAL_NUM(10)),
    T("s:=100 for i := -3:10 {s = s + 1}; return s", VAL_NUM(113)),

    // math
    T("return 3+4", VAL_NUM(7)),
    T("return 3-4", VAL_NUM(-1)),
    T("a:=-13; return -a", VAL_NUM(13)),
    T("return 3 * 4", VAL_NUM(12)),
    T("return 6 * 0.5", VAL_NUM(3)),
    T("return 3 / 0.5", VAL_NUM(6)),
    T("return 8 % 3.5", VAL_NUM(1)),
    T("return 2 ^ 10", VAL_NUM(1024)),

    T("return  1 << 10", VAL_NUM(1024)),
    T("return -1 <<  1", VAL_NUM((unsigned)-2)),
    T("return  1 << 32", VAL_NUM(0)),
    T("return ~0", VAL_NUM((unsigned)-1)),
    T("a:=~0 b:=2^32-1 return a==b", TRUE),
    T("a:=~0 b:=2^32-1 return a==b && ~(b-1)==1", TRUE),

    // recursion
    T("func f(n) { if n <= 0 {return 1} else {return n*f(n-1)}}; return f(10)", VAL_NUM(3628800)),
    T("func f(n) { if n <= 0 {return 1} else {return n + f(n-1)}}; return f(1000000)", VAL_NUM(500000500001)),
};

Value eval(const char *text) {
    Func *f = Parser::parseStatList(text);
    return f ? VM().run(f) : NIL;
}

bool compileDecompile(const char *text) {
    printf("\n'%s'\n\n", text);
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
        return 0;
    } else {
        int n = sizeof(tests) / sizeof(tests[0]);
        int nFail = 0;
        for (int i = 0; i < n; ++i) {
            T &t = tests[i];
            Value ret = eval(t.source);
            if (!equals(ret, t.result)) {
                fprintf(stderr, "\n%2d FAIL '%s'\n", i, t.source);
                printValue(ret);
                printValue(t.result);
                fprintf(stderr, "\n");
                ++nFail;
            } else {
                fprintf(stderr, "%2d OK '%s'\n", i, t.source);
            }
        }
        printf("\nPassed %d tests out of %d\n", (n-nFail), n);
        return nFail;
    }
}

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

    'var strlen = ffi("strlen", "int (const char *)"); return strlen("bar hello foo")'
    'var p = ffi("printf", "void (const char *, ...)"); var a=5; p("hello %d\n", a)'
    "var a = 5; return a * 2",
    */
    0,
};
