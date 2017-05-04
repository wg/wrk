-- example script that demonstrates possibility to collect
-- requests statistics and create custom reports
-- Usage:
-- wrk http://localhost --script ./scripts/plot.lua ./report.html

local counter = 1
local threads = {}

function setup(thread)
    thread:set("id", counter)
    table.insert(threads, thread)
    counter = counter + 1
end

function init(args)
    reportFile = './report.html'
    if args[1] ~= nil then
        reportFile = args[1]
    end
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

function response(status_, headers, body)
    table.insert(requestsTime, (os.clock() - requestStartTime) * 100000)
    table.insert(requestsStatus, status_)
end

function requestIterator(elapsed, time, status)
    local i = 0
    local n = table.getn(elapsed)
    return function ()
        i = i + 1
        if i <= n then
            return elapsed[i], time[i], status[i]
        end
    end
end

function getTemplateParts()
    local path = string.match(debug.getinfo(1,'S').source:sub(2), "^(.*/)[^/]+")
    local file = io.open(path .. 'plot-template.html', "rb")
    local template = file:read("*all")
    local startFirstPart = 0
    local endSecondPart = string.len(template)
    local endFirstPart, startSecondPart = string.find(template, "/* data */", 0, true)
    file:close()
    return template, startFirstPart, endFirstPart - 1, startSecondPart + 1, endSecondPart
end

function done()
    local template, startFirstPart, endFirstPart, startSecondPart, endSecondPart = getTemplateParts()

    local report = nil

    for index, thread in ipairs(threads) do
        if index == 1 then
            report = io.open(thread:get("reportFile"), "w+")
            report:write(string.sub(template, startFirstPart, endFirstPart))
        end
        local requestsElapsed = thread:get("requestsElapsed")
        local requestsTime = thread:get("requestsTime")
        local requestsStatus = thread:get("requestsStatus")
        for elapsed, time, status in requestIterator(requestsElapsed, requestsTime, requestsStatus) do
            if status == 200 then
                report:write("[" .. string.format('%.5f', elapsed) .. ",NaN," .. string.format('%.5f', time) .."],")
            else
                report:write("[" .. string.format('%.5f', elapsed) .. "," .. string.format('%.5f', time) ..",NaN],")
            end
        end
    end
    report:write(string.sub(template, startSecondPart, endSecondPart))
    report:close()
end
