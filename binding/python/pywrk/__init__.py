import os
from typing import TypedDict
from urllib.parse import urlparse
from ctypes import (
    Structure,
    c_char_p,
    c_uint64,
    c_void_p,
    cdll,
)

__version__ = "1.0.0"

_lib = os.path.join(os.path.dirname(__file__), "wrk.so")


class Request(TypedDict):
    url: str
    method: str
    data: str
    headers: dict


class Config(TypedDict):
    connections: int
    duration: int
    threads: int
    timeout: int


class c_config(Structure):
    _fields_ = [
        ("connections", c_uint64),
        ("duration", c_uint64),
        ("threads", c_uint64),
        ("timeout", c_uint64),
        ("host", c_char_p),
        ("ctx", c_void_p),
    ]


def benchmark(request: Request, config: Config):
    wrk = cdll.LoadLibrary(_lib)

    # prepare config and request
    cfg = c_config.in_dll(wrk, "cfg")
    http_message = c_char_p.in_dll(wrk, "request")

    cfg.connections = config.get("connections", 10)
    cfg.duration = config.get("duration", 3)
    cfg.threads = config.get("threads", 3)
    cfg.timeout = config.get("timeout", 1)

    url = request.get("url", "http://localhost")

    url_o = urlparse(url)
    if "headers" in request:
        headers_str = "\r\n" + "\r\n".join(request.get("headers", {}).items())
    else:
        headers_str = ""
    http_message.value = f"{request.get('method', 'GET')} / HTTP/1.1\nHost: {url_o.hostname}:{url_o.port}{headers_str}\r\n\r\n".encode(
        "utf-8"
    )

    wrk.benchmark(url.encode("utf-8"))
