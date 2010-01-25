require ('blocks')

print("Starting up...");

p = blocks.spawn(function() print 'hi there from a different thread of execution' return function(a,b,c) print (a .. ', ' .. b .. ', ' .. c) end end)

r = p:send(10, 1.223, 'Hello there', {fisk = 2}, function(a) a() end)
--print (r:get())

port = 8889
