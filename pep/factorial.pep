var fact
fact = func(n) {
     if !n { return 1 }
     else { return n * fact(n-1) }
}
return fact(10)
