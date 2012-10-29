def out():
    x=0
    def a():
        nonlocal x
        x += 1
        return x
    def b():
        nonlocal x
        x += 1
        return x
    x = 5
    return (a, b)

a, b = out()
print(a())
print(a())
print(b())
