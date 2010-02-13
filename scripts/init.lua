require ('blocks')
require ('io')

print("Starting up...");
handler_setup = function() 
    handler = function(path, query, sd)
        blocks.sleep(1);
        print ('D: sent: ', sent, ' recv: ', recv);
        print ('path:          ' , path)
        print ('query:         ', query)
        print ('client socket: ', sd)
        return handler
    end
    return handler
end
mbox = blocks.spawn(handler_setup)

dispatch = function(path, query, sd)
    fh = io.popen('ls');
    for line in fh:lines() do
        print (line)
        print (sd)
	    sd:write(line)
    end
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
		end end, {3,5, {1,2}, function()end, {1}, {a={b}}}, {a=2, c=true, b='test'})
		--end end, function()end, 3, true, 'testing', {1,2,4,5}, {a='test', b='another test', c={}} )

r = p:send(10, 1.223, 'Hello there', true, false, {fisk = 2}, function(a) a() end)
--print (r:get())

port = 8889

io.popen('notify-send started...')
