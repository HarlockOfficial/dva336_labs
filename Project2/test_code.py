#!/usr/bin/python3
import subprocess
import os

COMMAND_PAR = ["/home/administrator/Desktop/dva336_labs/Project2/pi_approximation_par.out"]
COMMAND_SEQ = ["/home/administrator/Desktop/dva336_labs/Project2/pi_approximation_seq.out"]


def main():
    i = 32768
    while True:
        runnable_par = COMMAND_PAR.copy()
        runnable_par.append(str(i))
        result_par = subprocess.run(runnable_par, stdout=subprocess.PIPE, shell=False)
        del runnable_par

        runnable_seq = COMMAND_SEQ.copy()
        runnable_seq.append(str(i))
        result_seq = subprocess.run(runnable_seq, stdout=subprocess.PIPE, shell=False)
        del runnable_seq
        arr_seq = result_seq.stdout.decode().split(": ")[1].split(" in ")
        arr_par = result_par.stdout.decode().split(": ")[1].split(" in ")
        with open("test_code_output.txt", "a") as output_file:
            output_file.write("with N=" + str(i) + ":\n" +
                              "\tsequential took: " + arr_seq[1][:-1] + " ms and got result: " + arr_seq[0] + "\n" +
                              "\tparallel took: " + arr_par[1][:-1] + " ms and got result: " + arr_par[0] + "\n" +
                              "----------------------------------------------------\n")
        with open("test_code_output.csv", "a") as output_csv:
            output_csv.write(str(i) + "," +
                             arr_seq[1][:-1] + "," + arr_par[1][:-1] + "," +
                             arr_seq[0] + "," + arr_par[0] + "\n")

        print(str(i))
        i += 32768


if __name__ == "__main__":
    # os.unlink("./test_code_output.txt")
    # os.unlink("./test_code_output.csv")
    main()
