local wrk = {
   scheme  = "http",
   host    = "localhost",
   port    = nil,
   method  = "GET",
   path    = "/",
   headers = {},
   body    = nil,
   thread  = nil,
}

function wrk.resolve(host, service)
   local addrs = wrk.lookup(host, service)
   for i = #addrs, 1, -1 do
      if not wrk.connect(addrs[i]) then
         table.remove(addrs, i)
      end
   end
   wrk.addrs = addrs
end

function wrk.setup(thread)
   thread.addr = wrk.addrs[1]
   if type(setup) == "function" then
      setup(thread)
   end
end

function wrk.init(args)
   if not wrk.headers["Host"] then
      local host = wrk.host
      local port = wrk.port

      host = host:find(":") and ("[" .. host .. "]")  or host
      host = port           and (host .. ":" .. port) or host

      wrk.headers["Host"] = host
   end

   if type(init) == "function" then
      init(args)
   end

   local req = wrk.format()
   wrk.request = function()
      return req
   end
end

function wrk.format(method, path, headers, body)
   local method  = method  or wrk.method
   local path    = path    or wrk.path
   local headers = headers or wrk.headers
   local body    = body    or wrk.body
   local s       = {}

   if not headers["Host"] then
      headers["Host"] = wrk.headers["Host"]
   end

   headers["Content-Length"] = body and string.len(body)

   s[1] = string.format("%s %s HTTP/1.1", method, path)
   for name, value in pairs(headers) do
      s[#s+1] = string.format("%s: %s", name, value)
   end

   s[#s+1] = ""
   s[#s+1] = body or ""

   return table.concat(s, "\r\n")
end

return wrk
