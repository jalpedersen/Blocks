require 'blocks'

p = blocks.spawn(
  function() 
    answer = function(args) 
      print ('You sent me: ')
      print (args)
      return answer
    end
    return answer			
  end
)
