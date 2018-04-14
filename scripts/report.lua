-- example reporting script which demonstrates a custom
-- done() function that prints latency percentiles as CSV

done = function(summary, latency, requests)
   io.write("------------------------------\n")
   io.write("Percentile\t|\tValue\n")
   io.write("------------------------------\n")
   for _, p in pairs({ 25, 75, 50, 90, 99, 99.999 }) do
      n = latency:percentile(p)
      io.write(string.format("p%g\t\t|\t%d\n", p, n))
   end
   io.write("------------------------------\n")
end
