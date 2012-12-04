function f(n)
  if n <= 0 then return 1 end
  return n + f(n-1)
end
print(f(300000))
