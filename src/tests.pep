tests1 := [
    ["return []", []],
    ["a := '[foo]' return a", 'foo'],
    ["b := 1 return b + 3", 4],
    ["return 13", 13],
    ['fn f(a) { a=2 { a:=3 { return a + 1 }}} return f(5)', 4],
    ['return [1]', [1]],
    ['a:=2 return [a]', [2]],
    ['a:=2 b:=[a] return b[0]', 2],
    ['a:=[] return [#a]', [0]],
    ['m:={a=2,b=3} m2:={}+m m2.a=nil return [#m, #m2]', [2, 1]],
]

tests := import('gen')
ok := 1
print(#tests, #tests1)
tests = tests + tests1
for i := 0 : #tests {
    t := tests[i]
    f := parse.block(t[0])
    res := f ? f() : nil
    if res != t[1] {
        ok = 0
        print('#'+i, 'expected ' + t[1] + ' got', res, t[0])        
    }
}
print('N ' + #tests)
return ok
