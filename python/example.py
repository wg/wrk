from pprint import pprint
from pywrk import benchmark

if __name__ == '__main__':
    result = benchmark(
        url="https://google.com",
        connections=100,
        threads=12,
        duration=1,
        stat_latency_percentile=[50,99],
    )
    pprint(result, indent=2)
