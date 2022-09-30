import multiprocessing 
from subprocess import PIPE, call


DEFAULT_ADDR = "http://192.168.1.67:8080"


def clear_logs_before_run() -> None:
    with open("log_info.txt", "w") as outfile:
        pass 


def run_wrk(
    addrs: str, 
    number_process: int,
    time: int
) -> None:
    print(f"Running wrk on {addrs}, process number: {number_process}")
    with open("log_info.txt", "a") as outfile:
        call(
            f"./wrk -t12 -c400 -d{time}s {addrs}", 
            stdout=outfile, 
            stderr=PIPE, 
            universal_newlines=True, 
            shell=True
        )



def run_multiprocess(
    wrks: int, 
    time: int
) -> None:
    process_list: list[multiprocessing.Process] = []
    for i in range(wrks):
        process_list.append(
            multiprocessing.Process(
                target=run_wrk, 
                args=(DEFAULT_ADDR, i, time, )
            )
        )
    for process in process_list:
        process.start()


def log_analytics() -> None:
    total_rps: list[int] = []
    with open("log_info.txt", "r") as stdout_info:
        info: list[str] = stdout_info.readlines()
        for line in info:
            if "Requests/sec" in line:
                processed_line: str = line \
                                      .replace(" ","") \
                                      .replace("Requests/sec:","") \
                                      .replace("\n","")
                total_rps.append(float(processed_line))
    print(f"Total RPS is: {sum(total_rps)}")




if __name__ == '__main__':
    question: int = int(input("Do log analytics or stress?\nLog=0\nStress=1\nInput: "))
    if question:
        addr_change: str \
            = input("Input address of stress test (http or https) or skip to run on default addr")
        if addr_change == "":
            pass 
        else:
            DEFAULT_ADDR = addr_change
        clear_logs_before_run()
        run_multiprocess(
            int(input("Input number of wrks:\n")), 
            int(input("Input duration of test: \n"))
        )
    else:
        log_analytics()
