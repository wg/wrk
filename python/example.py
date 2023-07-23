from pprint import pprint
from pywrk import benchmark

if __name__ == '__main__':
    result = benchmark("https://google.com", duration=1, timeout=10000)
    pprint(result, indent=2)
