from pprint import pprint
from pywrk import benchmark

if __name__ == '__main__':
    result = benchmark(
        url="https://dummyjson.com/products/1",
        connections=100,
        threads=12,
        duration=10,
    )
    pprint(result, indent=2)
