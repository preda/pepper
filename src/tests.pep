tests := [
    ["a := '[foo]' return a", 'foo'],
    ["return 13", 13],
]

print := builtin.print
for i := 0 : #tests {
    t := tests[i]
    print(t[0], " ==> ", t[1])
}
