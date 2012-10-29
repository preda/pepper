def adder(k):
    def a(x):
        return k + x
    return a

fs = [lambda x:i+x for i in range(2)]
a = fs[0]
b = fs[1]
print(a(1), a(2))
print(b(1), b(2))

for i in range(2):
    def f(x):
        return i + x
    fs[i] = f

print(fs[0](1), fs[1](1))


def h():
    a = 1
    def f():
        nonlocal a
        a = 2
        def g():
            a = 3
        return g

    g = f()
    print(a)
    g()
    print(a)
h()
