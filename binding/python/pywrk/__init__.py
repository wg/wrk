import os
from ctypes import (
    Structure,
    c_bool,
    c_char_p,
    c_uint16,
    c_uint64,
    c_void_p,
    cdll,
    pointer,
)

__version__ = "1.0.0"


class config(Structure):
    _fields_ = [
        ("connections", c_uint64),
        ("duration", c_uint64),
        ("threads", c_uint64),
        ("timeout", c_uint64),
        ("pipeline", c_uint64),
        ("delay", c_bool),
        ("dynamic", c_bool),
        ("latency", c_bool),
        ("host", c_char_p),
        ("script", c_char_p),
        ("ctx", c_void_p),
    ]


class field_data(Structure):
    _fields_ = [
        ("off", c_uint16),
        ("len", c_uint16),
    ]


class http_parser_url(Structure):
    _fields_ = [
        ("field_set", c_uint16),
        ("port", c_uint16),
        ("field_data", field_data * 7),
    ]

    def __init__(self):
        self.port = c_uint16(0)
        self.field_set = c_uint16(0)

        init = [field_data(c_uint16(0), c_uint16(0)) for _ in range(7)]
        self.field_data = (field_data * 7)(*init)


def run(url: str):
    url = url.encode("ascii")
    so = os.path.join(os.path.dirname(__file__), "wrk.so")
    wrk = cdll.LoadLibrary(so)
    raw_headers = ["content-length: 1", "x-code: 1"]
    headers = (c_char_p * len(raw_headers))()
    headers[:] = [item.encode("utf-8") for item in raw_headers]

    cfg = config.in_dll(wrk, "cfg")
    cfg.connections = 10
    cfg.duration = 10
    cfg.threads = 3

    parts = http_parser_url()
    wrk.parse_url(url, pointer(parts))
    wrk.wrk_run(url, headers, parts)

