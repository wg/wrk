-- example script that demonstrates use of the clock module

function setup(thread)
    print("Current wall time in us:" .. string.format("%.0f", clock.realtime()))
    print("Current monotonic time in us:" .. string.format("%.0f", clock.monotonic()))
end