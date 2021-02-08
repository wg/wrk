local counter = 1
local threads = {}

function setup(thread)
    thread:set("id", counter)
    table.insert(threads, thread)
    counter = counter + 1
end

function init(args_)
    args = args_
    requestsElapsed = {}
    requestsTime = {}
    requestsStatus = {}
    requestStartTime = 0
    startTime = os.clock()
end

function request()
    requestStartTime = os.clock()
    table.insert(requestsElapsed, (os.clock() - startTime))
    return wrk.request()
end

function response(status, headers, body)
    table.insert(requestsTime, (os.clock() - requestStartTime) * 100000)
    table.insert(requestsStatus, status)
end

function requestIterator(elapsed, time, status, n)
    local i = 0
    if n == nil then
        n = table.getn(status)
    end
    return function ()
        i = i + 1
        if i <= n then
            return elapsed[i], time[i], status[i]
        end
    end
end

function done(summary, latency, requests)
    for index, thread in ipairs(threads) do
        if index == 1 then
            onStart(thread:get("args"))
        end
        local requestsElapsed = thread:get("requestsElapsed")
        local requestsTime = thread:get("requestsTime")
        local requestsStatus = thread:get("requestsStatus")
        for elapsed, time, status in requestIterator(requestsElapsed, requestsTime, requestsStatus, summary.requests) do
            onStats(elapsed, time, status)
        end
        io.write(os.clock() - thread:get('startTime'))
    end
    onEnd()
end