-- example script demonstrating sharding requests across multiple threads
-- and terminating only once all threads have completed their complement
-- of requests

local counter = 0
local nobj = 60000000

request = function()
    path = "/" .. (counter + ((nobj / wrk.nthreads) * wrk.threadid))
    counter = counter + 1
    return wrk.format(nil, path)
end

local respcounter = 0
response = function(status, headers, body)
    respcounter = respcounter + 1
    if respcounter == (nobj / wrk.nthreads) then
        return false
    end

    return true
end
