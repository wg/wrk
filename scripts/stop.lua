-- example script that demonstrates use of thread:stop()

local counter = 1

function response()
   if counter == 100 then
      wrk.thread:stop()
   end
   counter = counter + 1
end
