from typings import Optional

def benchmark(
    host: str, 
    connections: Optional[int], 
    duration: Optional[int], 
    timeout: Optional[int], 
    threads: Optional[int], 
    http_message: Optional[str]
): ...
