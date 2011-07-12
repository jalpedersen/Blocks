require ('blocks')
require ('io')

print("Starting up...");
handler_setup = function() 
    handler = function(path, query, sd)
        blocks.sleep(10);
        print ('D: sent: ', sent, ' recv: ', recv);
        print ('path:          ' , path)
        print ('query:         ', query)
        print ('client socket: ', sd)
        collectgarbage()
        return handler
    end
    return handler
end
mbox = blocks.spawn(handler_setup)

dispatch = function(path, query, sd)
    fh = io.popen('ls');
    print (sd)
    for line in fh:lines() do
        print (line)
	sd:write(line)
    end
    sd:write("\n");
    sd:close()
    print (mbox:send(path, query))
end
p = blocks.spawn(
	function(table, table2) print 'hi there from a different thread of execution'
		if (type(table) == 'table') then
	      		for k,v in pairs(table) do 
				print (k, ':', v);
		   	end
          		for k,v in pairs(table2) do 
             			print (k, ':', v);
           		end  
		end 
		   	
		return function(a,b,c,d,e,table) 
			print (a .. ', ' .. b .. ', ' .. c) 
			print (d);
			print (e);	
		end end, {3,5, {1,2}, function()end, {1}, {a={b, {p=993}}}}, {a=2, c=true, b='test'})

r = p:send(10, 1.223, 'Hello there', true, false, {fisk = 2}, function(a) a() end)
print (r:get())

p2 = blocks.spawn(function() return function() return 42 end end) 
r2 = p2:send('testing')
print (r2:get())

port = 8889

io.popen('notify-send started...')
