tests := [
    ["a := '[foo]' return a", 'foo'],
    ["b := 1 return b + 3", 4],
    ["return 13", 13],
]

print := builtin.print
block := builtin.parse.block
ok := 1
for i := 0 : #tests {
    t := tests[i]
    f := block(t[0])
    res := f()
    if res != t[1] {
        ok = 0
        print('#'+i, 'expected ' + t[1] + ' got', res, t[0])        
    }
}
return ok
