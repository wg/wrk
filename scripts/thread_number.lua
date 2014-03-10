-- ./wrk -t 2 -d 1s -s scripts/thread_number.lua "http://127.0.0.1:1234/"

request = function()
   path = "/"
   wrk.headers["X-Thread"] = wrk.thread
   return wrk.format(nil, path)
end
