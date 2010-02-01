require ('blocks')

print("Starting up...");

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
