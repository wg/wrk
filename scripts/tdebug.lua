
function setup (thread)
	print ("setup >> " .. thread.tindex .. " of " .. thread.tcount)
end

function init(x)
	print ("init  >> " .. wrk.thread.tindex .. " of " .. wrk.thread.tcount)
end
