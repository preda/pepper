/*
func f(n, acc) {
     if n <= 0 { return acc }
     return f(n-1, n+acc)
}
*/

func fact(n) {
func f(n, acc) {
     if n <= 0 { return acc }
     return f(n-1, n+acc)
}
return f(n, 1)
}
