require 'blocks'

local first_child = blocks.spawn(
    function(a)
        f = function() 
            print (a)
            a = a + 1
            blocks.sleep(1)
            return f
        end 
        return f
    end,
    10)
first_child:send('hello')
io.read()
    
        