tests1 := [
    ["a := '[foo]' return a", 'foo'],
    ["b := 1 return b + 3", 4],
    ["return 13", 13],
]

tests := [
['=[a := '[foo]' return a == 'foo']=', 1],
['=[foobarbar := '====[tralala"']====' return foobarbar == '[tralala"']']=', 1],
['[f := builtin.parse.block('return 3') return f()]', 3],
['[f := builtin.parse.block('=[return 3]=') return f()]', 3],
['return nil', nil],
['return -1', -1],
['return 0', 0],
['return ""', ''],
['return #[]', 0],
['return #{}', 0],
['f:=fn() => 1 return f()', 1],
['f:=fn(x) => x+1; return f(-1)', 0],
['f:=fn(x) { return -1 } return f(5)', -1],
['f:=fn(x,y) { if x { return y } else { return 0 } }; return 1+f(0, 2)', 1],
['f:=fn(x,y) { if x { return y } else { return 0 } }; return 1+f(1, -1)', 0],
['return 13', 13],
['a := 10; b :=-2 c:=a+ b; return c', 8],
['a:=3; b:=a; return a', 3],
['a:=2; a=3; b:=a; return a', 3],
['[a := "foo" b := "bar" return a + b == 'foobar']', 1],
['return "foo" < "bar"', 0],
['return -3 < -2', 1],
['return "aaaaaaaa" < "b"', 1],
['return [] < [1] && [1] < [2]', 1],
['a:=2 b:=3 c:=nil return a+1 && b+2 || c+3', 5],
['return 1 && nil && 13', nil],
['return 13 || 14', 13],
['a:=0 b:=3 c:=4 return a || b || c', 3],
['a:=0 b:=3 c:=4 return a || (b && a) || c', 4],
['a:=0 b:=3 c:=4 return a && b && c', 0],
['a:=0 b:=3 c:=4 return b && c && a', 0],
['a:=0 b:=3 c:=4 return a || c && b', 3],
['a:=0 b:=3 c:=4 return (b || a) && (a || c)', 4],
['a:=[]; a[1]=2; b:=[]; return b[1]', nil],
['return #"foo"==3', 1],
['tralala:="tralala" return #tralala', 7],
['return #[]', 0],
['a:={} return #a-1', -1],
['a:=[4, 5]; return #a', 2],
['foofoo:={a=4, barbar=5, "1":"2"}; return 2*#foofoo', 6],
['s6:="abcabc" if s6[6]==nil and s6[5]=="c" { return 7 } else{return 8}', 7],
['a:=3; return a', 3],
['a:=2.5 b:=2*a return b+1', 6],
['i:=0; s:=0 while i < 4 { i = i + 1; s = s + i + 1 } return s', 14],
['i:=0 s:=0 while i < 100000 { s=s+i+1 i=i+1 } return s', 5000050000],
['i:=0 s:=0 while i < 100000 { s=s+i+0.5 i=i+0.5 } return s', 10000050000],
['s:=0 for i:=0:5 {s = s + i}; return s', 10],
['s:=100 for i := -3:10 {s = s + 1}; return s', 113],
['return 3+4', 7],
['return 3-4', -1],
['a:=-13; return -a', 13],
['return 3 * 4', 12],
['return 6 * 0.5', 3],
['return 3 / 0.5', 6],
['return 8 % 3.5', 1],
['return 2 ^ 10', 1024],
['return  1 << 10', 1024],
['return -1 <<  1', 4294967294],
['return  1 << 32', 0],
['return ~0', 4294967295],
['a:=~0 b:=2^32-1 return a==b', 1],
['a:=~0 b:=2^32-1 return a==b && ~(b-1)==1', 1],
['fn f(n) { if n <= 0 {return 1} else {return n*f(n-1)}}; return f(10)', 3628800],
['fn f(n) { if n <= 0 {return 1} else {return n + f(n-1)}}; return f(20000)', 200010001],
['return 1 ? 2 : 3', 2],
['a:=0 b:=10 c:=20 return a ? b+1 : c+2', 22],
['fn f(x) { return x + 10; } fn g(x, y) { return x * y } return g(3, 0) ? f(5) : g(f(0), f(1))', 110],
['return 0 ? 1 : 2 ? 3 : 4', 3],
['a:=2 return 0 ? 1 : a ? 3 : 4', 3],
['return {} === {}', 1],
['a:={} b:={} return a===b', 0],
['return "a" === "a"', 1],
['return "foofoobar" === "foofoobar"', 0],
['return [1] === [1]', 0],
['return 13 !== 13', 0],
['a := [3, 4, 5]; return a[2]', 5],
['foobar := [3, 4, 5] return [3, 4, 5] == foobar', 1],
['[tralala := [13, 14] tralala[3]="tralala"; return tralala[3]=='tralala']', 1],
['a:={} b:={} a[1]=13 return b[1]', nil],
['return {"foo" : 13}["foo"]', 13],
['a := {"foofoobar" : 13}; return a["foofoobar"]', 13],
['a := {}; a[2]=4; a[3]=9; return a[2]', 4],
['a := {}; a[2]=4; a[3]=9; return a ==  {2:4, 3:9}', 1],
['a := {}; a[2]=4; a[3]=9; return a === {2:4, 3:9}', 0],
['a := {}; b := fn(x){ a[x-1]=x+1 }; b(0); b(5); return a[4]-a[-1]', 5],
['return {a = 7}["a"]', 7],
['a:={foo=5, bar=7, 9:13} return a == {"foo":5, "bar":7, 9:13}', 1],
['a:={foo=7}; return a.foo', 7],
['a:={} a.foofoobar=5; return a["foofoobar"]', 5],
['a := [3, 4, 5]; return a[2:3] == [5]', 1],
['a := [3, 4, 5]; return a[0:-1] == [3, 4]', 1],
['return #([3, 4, 5][1:10])', 2],
['return [3, 4, 5][-3:-2] == [3]', 1],
['return "foo"[-5:-1]', 'fo'],
['return "aoeuaoeu"[-100:1] == "a"', 1],
['[a:="foobarrar" b:=3 c:=a[b:-b] d:= c + c; return 'barbar'==d]', 1],
['fn f(x) { return this.n + x } a:={n=5, foo=f}; return a.foo(3)', 8],
['return ({f=fn() { return this ? 3 : 4 }}.f)()', 4],
['return {f=fn() { return this ? this.bar+1 : 4 }, bar=5}.f()', 6],
['return ({f=fn() { return this ? 2*this.barbar : 4 }, barbar=5}).f()', 10],
['[f:=''.find; return f("hayneedfoo", "need")]', 3],
['s:="hayneedfoo"; return s.find("need")', 3],
['return "hayneedfoo".find("needl")', -1],
['s:="hayneedfoo"; return s.find()', nil],
['[return 'foo' == "foo"]', 1],
['return fn(*args) { return #args }([13, 15])', 1],
['fn foobar(*args) { return args[-1] } return foobar(13, 15)', 15],
['fn sum(*args) { s:=0; for i := 0:#args { s = s + args[i] }; return s }; return sum(3, 5, 7)', 15],
['fn sum(a, b) { return a + 2 * b } args := [3, 4] return sum(*args)', 11],
['fn foo(a, b, *args) { return a + b + #args} return foo(1, *[10, 7, 9])', 13],
['[builtin.print(nil, 'hello world', 1.5, 100, {3:9, 4:16, 'foo':"bar"}, [5, 4, 3]);]', nil],
['return #(builtin.type(builtin))', 3],
['[t:=builtin.type; return t('')=='string' && t(0)=='number']', 1],
['[m:={'a':7, '__get':{'c':8}}; return m.c]', 8],
['[m:={__get={'c':3}, a=4}; return m.c]', 3],
['m:={__get=fn(x){ return #x }, bar=2}; return m.foofoo', 6],
['[m:={__get=fn(x){ return #x }, bar=2}; return m['foofoo']]', nil],
['m:={2:3, __get=fn(x){ return #x }, bar=4}; return m.bar', 4],
['m:={2:3, __get={a=7, __get=fn(x){return 1+#x}}, bar=4}; return m.a', 7],
['m:={2:3, __get={a=7, __get=fn(x){return 1+#x}}, bar=4}; return m.ab', 3],
['[javaString := builtin.java.class('java/lang/String'); return 42]', 42],
['f:=fn(x) { return this}; builtin.print(f); return 1', 1],
['d:={a=2}; builtin.print(d.x)', nil],
['[p:=builtin.print; d:={__get=fn(x){ p('**', this, x); return this}}; builtin.print(d.x)]', nil],
['[p:=builtin.print; p('foo'); pp:=builtin.print; pp('pp'); builtin.print('bar'); p(p, pp, builtin.print)]', nil],
['d:={f=fn(){return this}}; p:=builtin.print; p((d).f(), (d.f)())', nil],
['fn a(x) { b:=fn(y) { c:=y+x; d:=fn(a) { p:=builtin.print; p(a)} d(c) } b(x+1)} a(3)', nil],
['[f:=builtin.parse.block('return nil'); return f()]', nil],
['b:=1 for i:=0:2 { a:=[4,5,6] b=a[2] a[2]=13} return b', 6],
['a:=[] a[3]=2 return a[3]', 2],
['a:=[] a[3]=2 b:=[] return b[3]', nil],
['a:={} a[3]=2 return a[3]', 2],
['a:={} a[3]=2 b:={} return b[3]', nil],

]

print := builtin.print
parse := builtin.parse.block
ok := 1
for i := 0 : #tests {
    t := tests[i]
    f := parse(t[0])
    res := f ? f() : nil;
    if res != t[1] {
        ok = 0
        print('#'+i, 'expected ' + t[1] + ' got', res, t[0])        
    }
}
return ok
