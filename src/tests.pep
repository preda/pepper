tests1 := [
    ["a := '[foo]' return a", 'foo'],
    ["b := 1 return b + 3", 4],
    ["return 13", 13],
    ['fn f(a) { a:=2 { a:=3 { return a + 1 }}} return f(5)', 4]
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
