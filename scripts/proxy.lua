-- execute as ./wrk [options] http://proxy:port -s proxy.lua -- http://target.website:port
init = function(args)
    target_url = args[1] -- proxy needs absolute URL
end

request = function()
    return wrk.format("GET", target_url)
end
