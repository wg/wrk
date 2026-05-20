
-- 4T / 256k
local max = 16 * 1000 * 1000

-- 50G / 256k
local hot = 200 * 1000

math.randomseed(os.time())

wrk.scheme  = "http"
wrk.host    = "127.0.0.1"
wrk.port    = 8017
wrk.method  = "GET"

headers = {
    ["Host"] = "test.com",
    ["Oct-Host"] = "192.168.3.137",
    ["Oct-expires-default"] = "5",
    ["Oct-Upstream-Retry"] = "0,2,504 502",
}

num = 0

local counter = 1
local threads = {}

function setup(thread, cs)
   print(string.format("setup cs=%d",cs))
   thread:set("id", counter)
   thread:set("connections", tonumber(cs))
   --print(string.format("setup connections=%d",connections))
   thread:set("request_count", 0);
   table.insert(threads, thread)
   counter = counter + 1
   --local msg = "setup id=%d, sc=%d, rc=%d\n"
   --print(msg:format(thread:get("id"), thread:get("connections"), thread:get("request_count")))
end


function init(args)
   requests  = 0
   responses = 0

   --local msg = "thread %d / %d created"
   --print(msg:format(id, wrk.parallel_worker))
end

function getlcm(m , n)
	local a = m
	local b = n
	while a~=b do
	   if a>b then  
		 a=a-b      
       else  
		 b=b-a
	   end
	end 
	return (m*n)/a
end

request = function(key)
	local lcm = getlcm(wrk.parallel_worker, connections)
    local localhot = 0
    local rand = math.random(0, max/wrk.parallel_worker)
    local single_hot = 0
    if rand % 100 < 100 then
        localhot = hot - hot % lcm
        single_hot = localhot/wrk.parallel_worker
		local single_pos = rand%single_hot - single_hot%connections
        rand = single_hot*(id-1) + single_pos + key

        headers["Oct-expires-default"] = "5"
    else
        localhot = (max-hot) - (max-hot) % lcm
        single_hot = localhot/wrk.parallel_worker
        local single_pos = rand%single_hot - single_hot%connections
        rand = hot+ single_hot*(id-1) + single_pos + key

        headers["Oct-expires-default"] = "5"
    end
 
    num = num + 1
    -- rand = num

    path = "/" .. '304file.html'
	--local msg = "request spec %d/%d %-6d %-8d/%4d %8d/%4d created\n"
	--print(msg:format(id, wrk.parallel_worker, single_hot, rand, key,localhot, lcm))
    return wrk.format("GET", path, headers)
end

--delay = function ()
--    return 20
    --return math.random(1, 500)
--end
