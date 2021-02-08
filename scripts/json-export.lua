-- example script that demonstrates possibility to collect
-- requests statistics and create custom reports

require("stats")

local template, startFirstPart, endFirstPart, startSecondPart, endSecondPart, reportFile, report;
function onStart(args)
    reportFile = './report.json'
    if args[1] ~= nil then
        reportFile = args[1]
    end
    report = nil
    report = io.open(reportFile, "w+")
    report:write(string.sub(template, startFirstPart, endFirstPart))
end

function onStats(elapsed, time, status)
    if status == 200 then
        report:write("[" .. string.format('%.5f', elapsed) .. ",NaN," .. string.format('%.5f', time) .."],")
    else
        report:write("[" .. string.format('%.5f', elapsed) .. "," .. string.format('%.5f', time) ..",NaN],")
    end
end

function onEnd()
    report:write(string.sub(template, startSecondPart, endSecondPart))
    report:close()
end
