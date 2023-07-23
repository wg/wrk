from typing import Optional, TypedDict


class WrkStat(TypedDict):
    limit: int
    min: int
    max: int
    count: int


class WrkResultStat(TypedDict):
    latency: WrkStat
    requests: WrkStat
    ttfb: WrkStat


class WrkErrors(TypedDict):
    connect: int
    timeout: int
    status: int
    read: int
    write: int


class WrkResult(TypedDict):
    completed_requests: int
    completed_bytes: int
    runtime_us: int
    errors: WrkErrors
    stats: WrkResultStat


def benchmark(
    host: str, 
    connections: Optional[int], 
    duration: Optional[int], 
    timeout: Optional[int], 
    threads: Optional[int], 
    http_message: Optional[str]
) -> WrkResult: ...
