require ('blocks')
require ('io')

print("Starting up...");
handler = function(path, query, sd)
    sent, recv = fileio.send(sd, 'hello there from dispatcher: ', sd, ' ', path, query, '\n')
    blocks.sleep(1);
    fileio.send(sd, 'closing ', sd, ' ', path, '\n');
--    fileio.close(sd)
    print ('D: sent: ', sent, ' recv: ', recv);
    print ('path:          ' , path)
    print ('query:         ', query)
    print ('client socket: ', sd)
end

dispatch = function(path, query, sd)
    fh = io.popen('ls');
    for line in fh:lines() do
  	fileio.send(sd, line, ' ')
    end
    local r = blocks.spawn(handler, path, query, sd);
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

io.popen('notify-send started...')
