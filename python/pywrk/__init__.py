from urllib.parse import urlparse
from dataclasses import dataclass
from typing import Optional, Union
from . import core

__all__ = ["benchmark"]


@dataclass
class WrkStat:
    limit: int
    min: int
    max: int
    count: int

@dataclass
class WrkResultStat:
    latency: WrkStat
    requests: WrkStat
    ttfb: WrkStat
    latency_percentile: list[int]

@dataclass
class WrkErrors:
    connect: int
    timeout: int
    status: int
    read: int
    write: int

@dataclass
class WrkResult:
    completed_requests: int
    completed_bytes: int
    runtime_us: int
    errors: WrkErrors
    stats: WrkResultStat

def benchmark(
    url: str,
    method: Optional[str] = "GET",
    headers: Optional[dict] = None,
    body: Optional[Union[str, bytes]] = None,
    connections: Optional[int] = 10, 
    duration: Optional[int] = 1, 
    timeout: Optional[int] = 3000, 
    threads: Optional[int] = 2, 
    stat_latency_percentile: Optional[list[float]] = [50, 75, 90, 99],
) -> WrkResult:
    incoming_message = generate_http_message(
        url,
        method,
        headers,
        body,
    )

    result = core.benchmark(
        url,
        connections,
        duration,
        timeout,
        threads,
        incoming_message,
        list(map(float, stat_latency_percentile)) if stat_latency_percentile else None,
    )

    return WrkResult(
        completed_requests=result["completed_requests"],
        completed_bytes=result["completed_bytes"],
        runtime_us=result["runtime_us"],
        errors=WrkErrors(**result["errors"]),
        stats=WrkResultStat(
            latency=WrkStat(**result["stats"]["latency"]),
            requests=WrkStat(**result["stats"]["requests"]),
            ttfb=WrkStat(**result["stats"]["ttfb"]),
            latency_percentile=result["stats"]["latency_percentile"],
        )
    )

def generate_http_message(
    url: str,
    method: Optional[str] = "GET",
    headers: Optional[dict] = None,
    body: Optional[Union[str, bytes]] = None,
    http_version: Optional[str] = "1.1",
) -> str:
    lines = []
    parsed_url = urlparse(url)

    if headers is None:
        headers = dict()
    headers["Host"] = parsed_url.netloc
    if body:
        headers["Content-Length"] = len(body)

    path = parsed_url.path
    if path:
        if parsed_url.query:
            path += f"?{parsed_url.query}"
    else:
        path = "/"

    for key, value in headers.items():
        lines.append(f"{key}: {value}")
    lines.append("")
    lines.append("")

    if body:
        lines.append(body)
    
    return f"{method} {path} HTTP/{http_version}\n" + "\r\n".join(lines)
