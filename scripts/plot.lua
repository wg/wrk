-- example script that demonstrates possibility to collect
-- requests statistics and create custom reports

require("stats")

local template, startFirstPart, endFirstPart, startSecondPart, endSecondPart, reportFile, report;

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

function onStart(args)
    reportFile = './report.html'
    if args[1] ~= nil then
        reportFile = args[1]
    end
    template, startFirstPart, endFirstPart, startSecondPart, endSecondPart = getTemplateParts()
    report = nil
    report = io.open(reportFile, "w+")
    report:write(string.sub(template, startFirstPart, endFirstPart))
end

function onStats(elapsed, time, status)
    local elapsed = string.format('%.5f', elapsed)
    local successReponseTime = "NaN"
    local errorResponseTime = "NaN"

    if time == nil then
        time = 0.0
    end

    if status >= 200 and status < 300 then
        successReponseTime = string.format('%.5f', time)
    else
        errorResponseTime = string.format('%.5f', time)
    end
    report:write("[" .. elapsed .. "," .. errorResponseTime .."," .. successReponseTime .. "],")
end

function onEnd()
    report:write(string.sub(template, startSecondPart, endSecondPart))
    report:close()
end
