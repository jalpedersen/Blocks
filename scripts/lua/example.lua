require 'blocks'

first_child = blocks.spawn(
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
print (first_child)
print (type(first_child))
reply = first_child:send('hello')
print (reply)
print (type(reply))
io.read()
