#!/usr/bin/python3
import subprocess
import sys

COMMAND_PAR = ["/home/administrator/Desktop/dva336_labs/Project2/pi_approximation_par.out"]
COMMAND_SEQ = ["/home/administrator/Desktop/dva336_labs/Project2/pi_approximation_seq.out"]


def main():
    i = 4096
    while True:
        runnable_par = COMMAND_PAR.copy()
        runnable_par.append(str(i))
        result_par = subprocess.run(runnable_par, stdout=subprocess.PIPE, shell=False)
        del runnable_par

        runnable_seq = COMMAND_SEQ.copy()
        runnable_seq.append(str(i))
        result_seq = subprocess.run(runnable_seq, stdout=subprocess.PIPE, shell=False)
        del runnable_seq
        print(result_par.stdout.decode())
        arr_seq = result_seq.stdout.decode().split(": ")[1].split(" in ")
        arr_par = result_par.stdout.decode().split(": ")[1].split(" in ")
        print("with N="+str(i)+":")
        print("\tsequential took: "+arr_seq[1][:-1]+" ms and got result: "+arr_seq[0])
        print("\tparallel took: "+arr_par[1][:-1]+" ms and got result: "+arr_par[0])
        print("----------------------------------------------------")
        sys.stderr.write(str(i)+"\n")
        i += 4096


if __name__ == "__main__":
    main()
