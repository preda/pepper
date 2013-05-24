tests := [
    ["a := '[foo]' return a", 'foo'],
    ["b := 1 return b + 3", 4],
    ["return 13", 13],
]

print := builtin.print
block := builtin.parse.block
for i := 0 : #tests {
    t := tests[i]
    f := block(t[0])
    res := f()
    print('#'+i, 'expected ' + t[1] + ' got ', res, t[0])
}
