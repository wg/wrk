-- example HTTP POST script which demonstrates setting the
-- HTTP method, body, and adding a header

local cjson = require "cjson"
local json_encode = cjson.encode
local json_decode = cjson.decode
wrk.method = "POST"
wrk.body = json_encode({["key"] = "value"})
wrk.headers["Content-Type"] = "application/x-www-form-urlencoded"
