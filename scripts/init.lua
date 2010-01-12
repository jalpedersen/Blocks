pcall(require ('blocks'))

print("Starting up...");

blocks.spawn(function() print 'hi there from a different thread of execution' end)