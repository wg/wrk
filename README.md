# wrk - a HTTP benchmarking tool

  wrk is a modern HTTP benchmarking tool capable of generating significant
  load when run on a single multi-core CPU. It combines a multithreaded
  design with scalable event notification systems such as epoll and kqueue.

  You also use it as library for programming purpose. wrk provides bindings 
  such as Python, NodeJS and Lua.

## Basic Usage

    wrk -d5s -t12 -c100 https://dummyjson.com/products/1

  This runs a benchmark for 30 seconds, using 12 threads, and keeping
  400 HTTP connections open.

  Output:

    Running 5s test @ https://dummyjson.com/products/1
      12 threads and 100 connections
      Thread Stats   Avg      Stdev     Max   +/- Stdev
        Latency    55.61ms   27.86ms   1.05s    97.92%
        Req/Sec   145.24     21.22   190.00     87.82%
        TTFB       54.48ms   13.68ms   1.05s    94.73%
      8608 requests in 5.10s, 11.81MB read
    Requests/sec:   1689.28
    Transfer/sec:      2.32MB

## Command Line Options

    -c, --connections: total number of HTTP connections to keep open with
                       each thread handling N = connections/threads

    -d, --duration:    duration of the test, e.g. 2s, 2m, 2h

    -t, --threads:     total number of threads to use

    -H, --header:      HTTP header to add to request, e.g. "User-Agent: wrk"

        --latency:     print detailed latency statistics

        --timeout:     record a timeout if a response is not received within
                       this amount of time.

## Benchmarking Tips

  The machine running wrk must have a sufficient number of ephemeral ports
  available and closed sockets should be recycled quickly. To handle the
  initial connection burst the server's listen(2) backlog should be greater
  than the number of concurrent connections being tested.

  A user script that only changes the HTTP method, path, adds headers or
  a body, will have no performance impact. Per-request actions, particularly
  building a new HTTP request, and use of response() will necessarily reduce
  the amount of load that can be generated.
