local i = 0
local s = 0
while i < 10000000 do
      s = s + i + 0.5
      i = i + 0.5
end
print(s)
