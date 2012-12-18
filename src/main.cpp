#include "Parser.h"
#include "Proto.h"
#include "SymbolTable.h"
#include "Decompile.h"
#include "VM.h"
#include "String.h"
#include "Array.h"
#include "Map.h"
#include "Pepper.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>

void printFunc(Func *);

struct T {
    T(const char *s, Value v) : source(s), result(v) {}

    const char *source;
    Value result;
};

Value eval(Pepper *pepper, const char *text) {
    Func *f = pepper->parseStatList(text);
    // printFunc(f);
    return f ? pepper->run(f) : NIL;
}

bool compileDecompile(Pepper *pepper, const char *text) {
    printf("\n'%s'\n\n", text);
    Func *f = pepper->parseStatList(text);
    if (f) { 
        printFunc(f); 
        Value ret = pepper->run(f);
        printValue(ret);
        return true;
    }
    return false;
}

static long long getTimeUsec() {
    timeval tv;
    timezone tz;
    gettimeofday(&tv, &tz);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

int main(int argc, char **argv) {
    Pepper *pepper = new Pepper();
    GC *gc = pepper->getGC();

T tests[] = {
    T("return nil", NIL),
    T("return -1", VAL_NUM(-1)),
    T("return 0", VAL_NUM(0)),
    T("return \"\"", EMPTY_STRING),
    T("return []", pepper->EMPTY_ARRAY),
    T("return {}", pepper->EMPTY_MAP),

    T("f:=func() { return 1 } return f()", ONE),
    T("f:=func(x) { return -1 } return f(5)", VAL_NUM(-1)),
    T("f:=func(x,y) { if x { return y } else { return 0 } }; return 1+f(0, 2)", ONE),
    T("f:=func(x,y) { if x { return y } else { return 0 } }; return 1+f(1, -1)", ZERO),

    T("return 13", VAL_NUM(13)),
    T("var a = 10; var b=-2 var c = a+ b; return c", VAL_NUM(8)),
    T("a:=3; b:=a; return a", VAL_NUM(3)),
    T("a:=2; a=3; b:=a; return a", VAL_NUM(3)),

    // strings
    T("var a = \"foo\" var b = \"bar\" return a + b", String::value(gc, "foobar")),

    // comparison
    T("return \"foo\" < \"bar\"", FALSE),
    T("return -3 < -2", TRUE),
    T("return \"aaaaaaaa\" < \"b\"", TRUE),
    T("return [] < [1] && [1] < [2]", TRUE),

    // bool ops
    T("var a=2 var b=3 var c=nil return a+1 && b+2 || c+3", VAL_NUM(5)),
    T("return 1 && nil && 13", NIL),
    T("return 13 || 14", VAL_NUM(13)),
    T("a:=0 b:=3 c:=4 return a || b || c", VAL_NUM(3)),
    T("a:=0 b:=3 c:=4 return a || (b && a) || c", VAL_NUM(4)),
    T("a:=0 b:=3 c:=4 return a && b && c", ZERO),
    T("a:=0 b:=3 c:=4 return b && c && a", ZERO),
    T("a:=0 b:=3 c:=4 return a || c && b", VAL_NUM(3)),
    T("a:=0 b:=3 c:=4 return (b || a) && (a || c)", VAL_NUM(4)),
    
    T("var a=[]; a[1]=2; var b=[]; return b[1]", NIL),
    
    // len
    T("return #\"foo\"==3", TRUE),
    T("var tralala=\"tralala\" return #tralala", VAL_NUM(7)),
    T("return #[]", ZERO),
    T("a:={} return #a-1", VAL_NUM(-1)),
    T("a:=[4, 5]; return #a", VAL_NUM(2)),
    T("foofoo:={a=4, barbar=5, \"1\":\"2\"}; return 2*#foofoo", VAL_NUM(6)),


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

    // ternary op
    T("return 1 ? 2 : 3", VAL_NUM(2)),
    T("a:=0 b:=10 c:=20 return a ? b+1 : c+2", VAL_NUM(22)),
    T("func f(x) { return x + 10; } func g(x, y) { return x * y } return g(3, 0) ? f(5) : g(f(0), f(1))", VAL_NUM(110)),
    T("return 0 ? 1 : 2 ? 3 : 4", VAL_NUM(3)),
    T("a:=2 return 0 ? 1 : a ? 3 : 4", VAL_NUM(3)),
    
    // is, not is
    T("return {} === {}", TRUE),
    T("a:={} b:={} return a===b", FALSE),
    T("return \"a\" === \"a\"", TRUE),
    T("return \"foofoobar\" === \"foofoobar\"", FALSE),
    T("return [1] === [1]", FALSE),
    T("return 13 !== 13", FALSE),

    // array
    T("var a = [3, 4, 5]; return a[2]", VAL_NUM(5)),
    T("var foobar = [3, 4, 5] return [3, 4, 5] == foobar", TRUE),
    T("var tralala = [13, 14] tralala[3]=\"tralala\"; return tralala[3]", String::value(gc, "tralala")),

    // map
    T("a:={} b:={} a[1]=13 return b[1]", NIL),
    T("return {\"foo\" : 13}[\"foo\"]", VAL_NUM(13)),
    T("a := {\"foofoobar\" : 13}; return a[\"foofoobar\"]", VAL_NUM(13)),
    T("a := {}; a[2]=4; a[3]=9; return a[2]", VAL_NUM(4)),
    T("a := {}; a[2]=4; a[3]=9; return a ==  {2:4, 3:9}", TRUE),
    T("a := {}; a[2]=4; a[3]=9; return a === {2:4, 3:9}", FALSE),
    T("a := {}; b := func(x){ a[x-1]=x+1 }; b(0); b(5); return a[4]-a[-1]", VAL_NUM(5)),
    T("return {a = 7}[\"a\"]", VAL_NUM(7)),
    T("a:={foo=5, bar=7, 9:13} return a == {\"foo\":5, \"bar\":7, 9:13}", TRUE),
    T("a:={foo=7}; return a.foo", VAL_NUM(7)),
    T("a:={} a.foofoobar=5; return a[\"foofoobar\"]", VAL_NUM(5)),
    
    // slice array
    T("var a = [3, 4, 5]; return a[2:3] == [5]", TRUE),
    T("var a = [3, 4, 5]; return a[0:-1] == [3, 4]", TRUE),
    T("return #([3, 4, 5][1:10])", VAL_NUM(2)),
    T("return [3, 4, 5][-3:-2] == [3]", TRUE),

    // slice string
    T("return \"foo\"[-5:-1]", String::value(gc, "fo")),
    T("return \"aoeuaoeu\"[-100:1] == \"a\"", TRUE),
    T("a:=\"foobarrar\" b:=3 c:=a[b:-b] return c + c", String::value(gc, "barbar")),
    
    // this call
    T("func f(x) { return this.n + x } a:={n=5, foo=f}; return a.foo(3)", VAL_NUM(8)),
    T("return ({f=func() { return this ? 3 : 4 }}.f)()", VAL_NUM(4)),
    T("return {f=func() { return this ? this.bar+1 : 4 }, bar=5}.f()", VAL_NUM(6)),
    T("return ({f=func() { return this ? 2*this.barbar : 4 }, barbar=5}).f()", VAL_NUM(10)),
    
    // string
    T("f:=string.find; return f(\"hayneedfoo\", \"need\")", VAL_NUM(3)),
    T("s:=\"hayneedfoo\"; return s.find(\"need\")", VAL_NUM(3)),
    T("return \"hayneedfoo\".find(\"needl\")", VAL_NUM(-1)),
    T("s:=\"hayneedfoo\"; return s.find()", NIL),
    T("return 'foo' == \"foo\"", TRUE),
    
};

    bool verbose = false;
    if (argc > 1 && !strcmp(argv[1], "-v")) {
        verbose = true;
        --argc;
        ++argv;
    }
    if (argc < 2) {
        int n = sizeof(tests) / sizeof(tests[0]);
        int nFail = 0;
        for (int i = 0; i < n; ++i) {
            T &t = tests[i];
            Value ret = eval(pepper, t.source);
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
    } else {
        const char *text;        
        if (!strcmp(argv[1], "-t")) {
            assert(argc > 2);
            int n = atoi(argv[2]);
            text = tests[n].source;
            eval(pepper, text);
            compileDecompile(pepper, text);
            ++argv;
            --argc;
            return 0;
        } else if (!strcmp(argv[1], "-f")) {
            assert(argc > 2);
            FILE *fi = fopen(argv[2], "rb");
            if (!fi) { return 2; }
            fseek(fi, 0, SEEK_END);
            size_t size = ftell(fi);
            fseek(fi, 0, SEEK_SET);    
            text = (const char *) mmap(0, size, PROT_READ, MAP_SHARED, fileno(fi), 0);
            fclose(fi);
            ++argv;
            --argc;
        } else {
            text = argv[1];
        }

        long long t1 = getTimeUsec();
        Func *f = pepper->parseFunc(text);
        if (verbose) {
            printFunc(f);
        }
        long long t2 = getTimeUsec();
        if (!f) {
            return 3;
        }
        Value fargs[5];
        int n = argc - 2;
        if (n > 5) { n = 5; }
        for (int i = 0; i < n; ++i) {
            fargs[i] = VAL_NUM(atoi(argv[2 + i]));                     
        }
        Value ret = pepper->run(f, n, fargs);
        long long t3 = getTimeUsec();
        printf("compilation %lld execution %lld\n", (t2-t1), (t3-t2));
        printValue(ret);
    }
}
