function fact(n) 
  function f(n, acc)
    if n <= 0 then return acc end
    return f(n-1, n+acc)
  end
  return f(n, 1)
end
