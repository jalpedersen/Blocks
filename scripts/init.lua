require ('blocks')

print("Starting up...");

p = blocks.spawn(function() print 'hi there from a different thread of execution' return function(a) print (a) end end)

r = p:send('Hello there')
--print (r:get())