local wrk = {
   scheme  = "http",
   host    = "localhost",
   port    = nil,
   method  = "GET",
   path    = "/",
   headers = {},
   body    = nil
}

function wrk.format(method, path, headers, body)
   local host    = wrk.host
   local method  = method  or wrk.method
   local path    = path    or wrk.path
   local headers = headers or wrk.headers
   local body    = body    or wrk.body
   local s       = {}

   headers["Host"]           = port and (host .. ":" .. port) or host
   headers["Content-Length"] = body and string.len(body)

   s[1] = string.format("%s %s HTTP/1.1", method, path)
   for name, value in pairs(headers) do
      s[#s+1] = string.format("%s: %s", name, value)
   end

   s[#s+1] = ""
   s[#s+1] = body or ""

   return table.concat(s, "\r\n")
end

function wrk.init(args) req = wrk.format() end
function wrk.request()  return req         end

init     = wrk.init
request  = wrk.request
response = nil
done     = nil

return wrk
