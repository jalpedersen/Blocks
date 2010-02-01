require ('blocks')
require ('io')

print("Starting up...");
handler = function(path, query, sd)
    print ('path: ' .. path)
    print ('query: '.. query)
end

dispatch = function(path, query, sd)
    netio.send(sd, 'hello there', 1, 'blah blah')
    netio.close(sd)
    
    blocks.spawn(handler, path, query, sd); 
end
p = blocks.spawn(
	function() print 'hi there from a different thread of execution' 
		return function(a,b,c,d,e) 
			print (a .. ', ' .. b .. ', ' .. c) 
			print (d);
			print (e);	
		end end)

r = p:send(10, 1.223, 'Hello there', true, false, {fisk = 2}, function(a) a() end)
--print (r:get())

port = 8889
