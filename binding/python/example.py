import pywrk

if __name__ == "__main__":
    request = {
        "url": "http://localhost:8000",
        "method": "GET",
    }
    config = {
        "connections": 100,
        "duration": 10,
        "threads": 12,
    }

    pywrk.benchmark(request, config)
