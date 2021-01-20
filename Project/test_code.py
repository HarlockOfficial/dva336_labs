#!/usr/bin/python3
import subprocess

EXECUTABLE_NAME_PAR = "/home/administrator/Desktop/dva336_labs/Project/dijkstra_par.out"
PROCESS_NUMBER = str(4)
HOSTFILE_NAME = "/home/administrator/Desktop/dva336_labs/Project/hostfile"
COMMAND_PAR = ["mpirun", "-np", str(PROCESS_NUMBER), "--hostfile", str(HOSTFILE_NAME), str(EXECUTABLE_NAME_PAR)]
COMMAND_SEQ = ["/home/administrator/Desktop/dva336_labs/Project/dijkstra_seq.out"]


def main():
    for i in range(10, 200, 10):
        runnable_par = COMMAND_PAR.copy()
        runnable_par.append(str(i))
        result_par = subprocess.run(runnable_par, stdout=subprocess.PIPE, shell=False)
        del runnable_par

        runnable_seq = COMMAND_SEQ.copy()
        runnable_seq.append(str(i))
        result_seq = subprocess.run(runnable_seq, stdout=subprocess.PIPE, shell=False)
        del runnable_seq

        out_par_list = result_par.stdout.decode().split("\n")
        out_seq_list = result_seq.stdout.decode().split("\n")
        del result_par
        del result_seq

        time_par = out_par_list.pop(0).split(":")[1][1:]
        time_seq = out_seq_list.pop(0).split(":")[1][1:]
        res_par = ("\n".join(out_par_list)).strip()
        res_seq = ("\n".join(out_seq_list)).strip()
        print("node count:", i, "parallel dijkstra:", time_par, "sequential dijkstra:", time_seq, "output equals: ",
              res_par == res_seq)
        del time_par
        del time_seq
        del res_par
        del res_seq


if __name__ == "__main__":
    main()
