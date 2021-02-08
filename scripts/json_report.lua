-- example script for creating json file report

done = function(summary, latency, requests)
    file = io.open('result.json', 'a')
    io.output(file)
    io.write(string.format("{\"requests_sec\":%.2f, \"transfer_sec\":%.2fMB, \"avg_latency_ms\":%.2f, \"errors_sum\":%.2f, \"duration\":%.2f,\"requests\":%.2f, \"bytes\":%.2f, \"latency.min\":%.2f, \"latency.max\":%.2f, \"latency.mean\":%.2f, \"latency.stdev\":%.2f}",
            summary.requests/(summary.duration/1000000),
            summary.bytes/(summary.duration*1048576/1000000),
            (latency.mean/1000),
            summary.errors.connect + summary.errors.read + summary.errors.write + summary.errors.status + summary.errors.timeout,
            summary.duration,
            summary.requests,
            summary.bytes,
            latency.min,
            latency.max,
            latency.mean,
            latency.stdev
        )
    )
end