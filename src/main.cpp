#include "Parser.h"
#include "Proto.h"
#include "Decompile.h"
#include "VM.h"
#include "String.h"
#include "Array.h"
#include "Map.h"
#include "Pepper.h"
#include "GC.h"
#include "Index.h"
#include "StringBuilder.h"
#include "Lexer.h"
#include "SpanTracker.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>

void testRegAlloc() {
    {
        SpanTracker spans;
        std::vector<int> varToReg = spans.varToReg(0, 0);
        assert(varToReg.size() == 0);
    }

    {
        SpanTracker spans;
        spans.write(0, 0);
        spans.write(1, 0);
        spans.write(2, 1);
        spans.write(3, 1);
        spans.read(0, 3);
        spans.read(1, 2);
        spans.read(2, 2);
        spans.read(3, 6);
        int args[] = {0, 1};
        std::vector<int> varToReg = spans.varToReg(2, args);
        assert(varToReg.size() == 4);
        assert(varToReg[2] == 3);
        assert(varToReg[3] == 2);
        for (int i = 0; i < 4; ++i) {
            printf("%d : %d\n", i, varToReg[i]);
        }
    }    
}

void testIndex() {
    Index idx;
    Value k1 = VAL_NUM(7);
    Value k2 = VAL_NUM(13);
    assert(idx.size() == 0);    

    assert(idx.add(k1) == -1);
    assert(idx.getPos(k1) == 0);
    assert(idx.getVal(0) == k1);
    assert(idx.add(k1) == 0);
    assert(idx.size() == 1);


    assert(idx.add(k2) == -1);
    assert(idx.add(k2) == 1);
    assert(idx.getPos(k2) == 1);
    assert(idx.getVal(1) == k2);
    assert(idx.size() == 2);

    assert(idx.remove(VAL_NUM(1)) == -1);
    assert(idx.size() == 2);
    assert(idx.remove(k1) == 0);
    assert(idx.getPos(k2) == 0);
    assert(idx.getVal(0) == k2);
    assert(idx.size() == 1);
    
}

static void printValue(Value a) {
    StringBuilder sb;
    sb.append(a);
    fprintf(stderr, "%s\n", sb.cstr());
}

struct T {
    T(const char *s, Value v) : source(s), result(v) {}

    const char *source;
    Value result;
};

Value eval(Pepper *pepper, const char *text) {
    Func *f = pepper->parseStatList(text);
    // printFunc(f);
    return f ? pepper->run(f) : VNIL;
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
    testIndex();
    testRegAlloc();
    Pepper *pepper = new Pepper(0);
    GC *gc = pepper->gc();

T tests[] = {
    // T("return []", VNIL),
    T("a := '[foo]' return a == 'foo'", TRUE),
    T("foobarbar := '====[tralala\"']====' return foobarbar == '[tralala\"']'", TRUE),
    T("f := parse.block('return 3') return f()", VAL_NUM(3)),
    T("f := parse.block('=[return 3]=') return f()", VAL_NUM(3)),
    T("return nil", VNIL),
    T("return -1", VAL_NUM(-1)),
    T("return 0", VAL_NUM(0)),
    T("return \"\"", EMPTY_STRING),
    T("return #[]", VAL_NUM(0)),
    T("return #{}", VAL_NUM(0)),

    T("f:=fn() => 1 return f()", ONE),
    T("f:=fn(x) => x+1 return f(-1)", ZERO),

    T("f:=fn(x) { return -1 } return f(5)", VAL_NUM(-1)),
    T("f:=fn(x,y) { if x { return y } else { return 0 } } return 1+f(nil, 2)", ONE),
    T("f:=fn(x,y) { if x { return y } else { return 0 } } return 1+f(1, -1)", ZERO),

    T("return 13", VAL_NUM(13)),
    T("a := 10 b :=-2 c:=a+ b return c", VAL_NUM(8)),
    T("a:=3 b:=a return a", VAL_NUM(3)),
    T("a:=2 a=3 b:=a return a", VAL_NUM(3)),

    // strings
    T("a := \"foo\" b := \"bar\" return a + b == 'foobar'", TRUE),

    // comparison
    T("return \"foo\" < \"bar\"", FALSE),
    T("return -3 < -2", TRUE),
    T("return \"aaaaaaaa\" < \"b\"", TRUE),
    T("return [] < [1] && [1] < [2]", TRUE),

    // bool ops
    T("a:=2 b:=3 c:=nil return a+1 && b+2 || c+3", VAL_NUM(5)),
    T("return 1 && nil && 13", VNIL),
    T("return 13 || 14", VAL_NUM(13)),
    T("a:=nil b:=3 c:=4 return a || b || c", VAL_NUM(3)),
    T("a:=nil b:=3 c:=4 return a || (b && a) || c", VAL_NUM(4)),
    T("a:=nil b:=3 c:=4 return a && b && c", VNIL),
    T("a:=nil b:=3 c:=4 return b && c && a", VNIL),
    T("a:=nil b:=3 c:=4 return a || c && b", VAL_NUM(3)),
    T("a:=nil b:=3 c:=4 return (b || a) && (a || c)", VAL_NUM(4)),
    
    T("a:=[] a[1]=2 b:=[] return b[1]", VNIL),
    
    // len
    T("return #\"foo\"==3", TRUE),
    T("tralala:=\"tralala\" return #tralala", VAL_NUM(7)),
    T("return #[]", ZERO),
    T("a:={} return #a-1", VAL_NUM(-1)),
    T("a:=[4, 5] return #a", VAL_NUM(2)),
    T("foofoo:={a=4, barbar=5, \"1\":\"2\"} return 2*#foofoo", VAL_NUM(6)),


    T("s6:=\"abcabc\" if s6[6]==nil and s6[5]==\"c\" { return 7 } else{return 8}", VAL_NUM(7)),

    // ffi
    // T("var strlen=ffi(\"strlen\", \"int (char *)\", \"c\") return strlen(\"bar hello foo\")", VAL_NUM(13)),
    // T("return ffi(\"strstr\", \"offset (char*, char *)\", \"c\")(\"big oneedle hayst\", \"need\")", VAL_NUM(5)),

    // assign
    T("a:=3 return a", VAL_NUM(3)),
    T("a:=2.5 b:=2*a return b+1", VAL_NUM(6)),

    // while
    T("i:=0 s:=0 while i < 4 { i = i + 1 s = s + i + 1 } return s", VAL_NUM(14)),
    T("i:=0 s:=0 while i < 50000 { s=s+i+1 i=i+1 } return s",     VAL_NUM(1250025000ll)),
    T("i:=0 s:=0 while i < 10000 { s=s+i+0.5 i=i+0.5 } return s", VAL_NUM(100005000ll)),

    T("s:=0 for i:=0:5 {s = s + i} return s", VAL_NUM(10)),
    T("s:=100 for i := -3:10 {s = s + 1} return s", VAL_NUM(113)),

    // math
    T("return 3+4", VAL_NUM(7)),
    T("return 3-4", VAL_NUM(-1)),
    T("a:=-13 return -a", VAL_NUM(13)),
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
    T("fn f(n) { if n <= 0 {return 1} else {return n*f(n-1)}} return f(10)", VAL_NUM(3628800)),
    T("fn f(n) { if n <= 0 {return 1} else {return n + f(n-1)}} return f(20000)", VAL_NUM(200010001)),

    // ternary op
    T("return 1 ? 2 : 3", VAL_NUM(2)),
    T("a:=nil b:=10 c:=20 return a ? b+1 : c+2", VAL_NUM(22)),
    T("fn f(x) { return x + 10 } fn g(x, y) { return x * y } return g(3, 0) != 0 ? f(5) : g(f(0), f(1))", VAL_NUM(110)),
    T("return nil ? 1 : 2 ? 3 : 4", VAL_NUM(3)),
    T("a:=2 return nil ? 1 : a ? 3 : 4", VAL_NUM(3)),
    
    // is, not is
    T("return {} === {}", FALSE),
    T("a:={} b:={} return a===b", FALSE),
    T("return \"a\" === \"a\"", TRUE),
    T("return \"foofoobar\" === \"foofoobar\"", FALSE),
    T("return [1] === [1]", FALSE),
    T("return 13 !== 13", FALSE),

    // array
    T("a := [3, 4, 5] return a[2]", VAL_NUM(5)),
    T("foobar := [3, 4, 5] return [3, 4, 5] == foobar", TRUE),
    T("tralala := [13, 14] tralala[3]=\"tralala\" return tralala[3]=='tralala'", TRUE),

    // map
    T("a:={} b:={} a[1]=13 return b[1]", VNIL),
    T("return {\"foo\" : 13}[\"foo\"]", VAL_NUM(13)),
    T("a := {\"foofoobar\" : 13} return a[\"foofoobar\"]", VAL_NUM(13)),
    T("a := {} a[2]=4 a[3]=9 return a[2]", VAL_NUM(4)),
    T("a := {} a[2]=4 a[3]=9 return a ==  {2:4, 3:9}", TRUE),
    T("a := {} a[2]=4 a[3]=9 return a === {2:4, 3:9}", FALSE),
    T("a := {} b := fn(x){ a[x-1]=x+1 } b(0) b(5) return a[4]-a[-1]", VAL_NUM(5)),
    T("return {a = 7}[\"a\"]", VAL_NUM(7)),
    T("a:={foo=5, bar=7, 9:13} return a == {\"foo\":5, \"bar\":7, 9:13}", TRUE),
    T("a:={foo=7} return a.foo", VAL_NUM(7)),
    T("a:={} a.foofoobar=5 return a[\"foofoobar\"]", VAL_NUM(5)),
    
    // slice array
    T("a := [3, 4, 5] return a[2:3] == [5]", TRUE),
    T("a := [3, 4, 5] return a[0:-1] == [3, 4]", TRUE),
    T("return #([3, 4, 5][1:10])", VAL_NUM(2)),
    T("return [3, 4, 5][-3:-2] == [3]", TRUE),

    // slice string
    T("return \"foo\"[-5:-1]", String::value(gc, "fo")),
    T("return \"aoeuaoeu\"[-100:1] == \"a\"", TRUE),
    T("a:=\"foobarrar\" b:=3 c:=a[b:-b] d:= c + c return 'barbar'==d", TRUE),
    
    // this call
    T("fn f(x) { return this.n + x } a:={n=5, foo=f} return a.foo(3)", VAL_NUM(8)),
    T("return ({f=fn() { return this ? 3 : 4 }}.f)()", VAL_NUM(4)),
    T("return {f=fn() { return this ? this.bar+1 : 4 }, bar=5}.f()", VAL_NUM(6)),
    T("return ({f=fn() { return this ? 2*this.barbar : 4 }, barbar=5}).f()", VAL_NUM(10)),
    
    // string
    T("f:=''.find return f(\"hayneedfoo\", \"need\")", VAL_NUM(3)),
    T("s:=\"hayneedfoo\" return s.find(\"need\")", VAL_NUM(3)),
    T("return \"hayneedfoo\".find(\"needl\")", VAL_NUM(-1)),
    T("s:=\"hayneedfoo\" return s.find()", VNIL),
    T("return 'foo' == \"foo\"", TRUE),
    
    // vararg
    T("return fn(*args) { return #args }([13, 15])", VAL_NUM(1)),
    T("fn foobar(*args) { return args[-1] } return foobar(13, 15)", VAL_NUM(15)),
    T("fn sum(*args) { s:=0 for i := 0:#args { s = s + args[i] } return s } return sum(3, 5, 7)", VAL_NUM(3+5+7)),
    T("fn sum(a, b) { return a + 2 * b } args := [3, 4] return sum(*args)", VAL_NUM(11)),
    T("fn foo(a, b, *args) { return a + b + #args} return foo(1, *[10, 7, 9])", VAL_NUM(13)),

    T("print(nil, 'hello world', 1.5, 100, {3:9, 4:16, 'foo':\"bar\"}, [5, 4, 3])", VNIL),

    // type()
    // T("return #(type(builtin))", VAL_NUM(3)), 
    T("t:=type return t('')=='string' && t(0)=='number'", TRUE),

    // GETF
    T("m:={'a':7, super={'c':8}} return m.c", VAL_NUM(8)),
    T("m:={super={'c':3}, a=4} return m.c", VAL_NUM(3)),
    // T("m:={__get=fn(x){ return #x }, bar=2} return m.foofoo", VAL_NUM(6)),
    // T("m:={__get=fn(x){ return #x }, bar=2} return m['foofoo']", VNIL),
    // T("m:={2:3, __get=fn(x){ return #x }, bar=4} return m.bar", VAL_NUM(4)),
    // T("m:={2:3, __get={a=7, __get=fn(x){return 1+#x}}, bar=4} return m.a", VAL_NUM(7)),
    // T("m:={2:3, __get={a=7, __get=fn(x){return 1+#x}}, bar=4} return m.ab", VAL_NUM(3)),
    
    // T("a:=java return 42", VAL_NUM(42)),
    T("javaString := java.class('java/lang/String') return 42", VAL_NUM(42)),
    T("f:=fn(x) { return this} print(f) return 1", VAL_NUM(1)),
    T("d:={a=2} print(d.x)", VNIL),
    T("p:=print d:={__get=fn(x){ p('**', this, x) return this}} print(d.x)", VNIL),
    T("p:=print p('foo') pp:=print pp('pp') print('bar') p(p, pp, print)", VNIL),
    T("d:={f=fn(){return this}} p:=print p((d).f(), (d.f)())", VNIL),
    T("fn a(x) { b:=fn(y) { c:=y+x d:=fn(a) { p:=print p(a)} d(c) } b(x+1)} a(3)", VNIL),
    T("f:=parse.block('return nil') return f()", VNIL),
    T("b:=1 for i:=0:2 { a:=[4,5,6] b=a[2] a[2]=13} return b", VAL_NUM(6)),
    T("a:=[] a[3]=2 return a[3]", VAL_NUM(2)),
    T("a:=[] a[3]=2 b:=[] return b[3]", VNIL),
    T("a:={} a[3]=2 return a[3]", VAL_NUM(2)),
    T("a:={} a[3]=2 b:={} return b[3]", VNIL),
    T("file.read='foofoobar' return #file.read", VAL_NUM(9)),
    T("f:=file.read('gen.pep') return !f || #f > 0", TRUE),
    T("print('import:', import) for i:=0:2 { f:=import('testimp') print('f:', i, f, f?f(i + 1):'*')} return import('testimp') ? import('testimp')() : 100", VAL_NUM(100)),
    T("m:={a=2,b=3} m2:={}+m m2.a=nil return #m2", VAL_NUM(1)),
    T("a:=2 b:=[a] return b[0]", VAL_NUM(2)),
    T("a:=[[]] a[1]=[] a[1][0]=2 b:=[] return b[0]", VNIL),
    T("a:=0 b:=0 for b:=1:3 { a = a + 2 } return b", ZERO),
    // T("a:={foo=['_prop', fn(key, prop){ print(this, key, prop) return '_'+key}, fn(key, v, prop){ }]} print(a.foo)", VNIL),
};

    bool verbose = false;
    if (argc > 1 && !strcmp(argv[1], "-v")) {
        verbose = true;
        --argc;
        ++argv;
    }
    const int n = sizeof(tests) / sizeof(tests[0]);
    if (argc < 2) {
        int nFail = 0;
        for (int i = 0; i < n; ++i) {
            T &t = tests[i];
            const char *txt = t.source;
            Value ret = eval(pepper, txt);
            char buf[512];
            if (!equals(ret, t.result)) {
                fprintf(stderr, "\n%2d FAIL '%s'\n", i, t.source);
                printValue(ret);
                printValue(t.result);
                fprintf(stderr, "\n");
                ++nFail;
                break;
            } else {
                fprintf(stderr, "%2d OK '%s'\n", i, t.source);
                printValue(ret);
            }
            snprintf(buf, sizeof(buf), "f:=parse.block('===[%s]===') return f()", txt);
            Value ret2 = eval(pepper, buf);
            if (!equals(ret, ret2)) {
                fprintf(stderr, "parse %d '%s'\n", i, txt);
                ++nFail;
                break;
            }
        }
        printf("\nFailed %d out of %d\n", nFail, n);
    } else {
        const char *text;        
        if (!strcmp(argv[1], "-t")) {
            assert(argc > 2);
            int n = atoi(argv[2]);
            text = tests[n].source;
            compileDecompile(pepper, text);
            Value ret = eval(pepper, text);
            ++argv;
            --argc;
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
        } else if (!strcmp(argv[1], "-s")) {
            printf("return [\n");
            StringBuilder builder;
            for (int i = 0; i < n; ++i) {
                const char *txt = tests[i].source;
                char buf[256];
                Lexer::quote(buf, sizeof(buf), txt);
                builder.clear();
                builder.append(tests[i].result);
                printf("[%s, %s],\n", buf, builder.cstr());
            }
            printf("]\n");
            return 0;
        } else {
            text = argv[1];
        }

        long long t1 = getTimeUsec();
        Func *f = pepper->parseStatList(text);
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
    delete pepper;
}
