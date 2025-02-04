import time
import subprocess
import random
import string

# execute a command and return its duration in ms
def execute_cmd():
    key = ''.join(random.choices(string.ascii_uppercase + string.digits, k=10))
    value = ''.join(random.choices(string.ascii_uppercase + string.digits, k=1000))

    start = time.time()
    subprocess.run(f"../.build/client 0.0.0.0 38287 {key} {value}", shell=True)#, stdout=subprocess.DEVNULL)
    end = time.time()
    return (end - start) * 1000

# execute a workload and return statistics about duration 
def execute_n_cmds(nb_executions):
    individual_times = []
    start = time.time()

    for _ in range(nb_executions):
        individual_times.append(execute_cmd())

    end = time.time()
    diff = (end - start) * 1000 #in ms
    print(f"Executed {nb_executions} times in {diff}ms ({diff / nb_executions}ms per execution)")
    print(f"Mean individual execution time is {sum(individual_times) / len(individual_times)}")

if __name__ == "__main__":
    execute_n_cmds(10000)
