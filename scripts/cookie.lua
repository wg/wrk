
function getCookie(cookies, name)
  local start = string.find(cookies, name .. "=")

  if start == nil then
    return nil
  end

  return string.sub(cookies, start + #name + 1, string.find(cookies, ";", start) - 1)
end


response = function(status, headers, body)
  local token = getCookie(headers["Set-Cookie"], "token")
  
  if token ~= nil then
    wrk.headers["Cookie"] = "token=" .. token
  end
end

