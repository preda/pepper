print((function(x)
        f = function()
          --x = 2
          local y = x
          g()
          return x, y
        end

        g = function()
          x = x + 1
        end

        return f
end)(5)())

