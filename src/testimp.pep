last := 0
fn f(x) { if x { last = last + 1 return last } else { return last > 1 ? 100 : 0 } }
return f
