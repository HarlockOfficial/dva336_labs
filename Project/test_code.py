#!/usr/bin/python3
import subprocess

EXECUTABLE_NAME = "/home/administrator/Desktop/dva336_labs/Project/dijkstra_par.out"
PROCESS_NUMBER = str(4)
HOSTFILE_NAME = "/home/administrator/Desktop/dva336_labs/Project/hostfile"
COMMAND = ["mpirun", "-np", str(PROCESS_NUMBER), "--hostfile", str(HOSTFILE_NAME), str(EXECUTABLE_NAME)]


def main():
    for i in range(10, 200, 10):
        runnable = COMMAND.copy()
        runnable.append(str(3))
        result = subprocess.run(runnable, stdout=subprocess.PIPE, shell=False)
        res = result.stdout.decode().split("\n")[0].split(":")[1][1:]
        print(res, "required to solve a graph with", i, "nodes")


if __name__ == "__main__":
    main()
