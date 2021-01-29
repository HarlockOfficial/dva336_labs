#!/usr/bin/python3
import math
from typing import Tuple

import matplotlib.pyplot as plt

division_factor_for_reference_values = math.pow(10, 4)/1.5
k = 4  # thread count


def main():
    x, y_seq, y_par, res_seq, res_par = get_data("test_code_output.csv")

    x_fixed = list()
    x_div_k = list()
    pi = list()
    for elem in x:
        x_fixed.append(elem / division_factor_for_reference_values)
        x_div_k.append((elem / k) / division_factor_for_reference_values)
        pi.append(math.pi)

    # plot time
    plt.figure(1)
    plt.plot(x, y_seq, label="Sequential Time")
    plt.plot(x, y_par, label="Parallel Time")
    plt.plot(x, x_fixed, label="N")
    plt.plot(x, x_div_k, label="N/k")
    plt.xlabel("Value of N")
    plt.ylabel("ms to calculate approximate value of PI")
    plt.legend()
    plt.gca().set_ylim([0, 11000])

    # plot results
    plt.figure(2)
    plt.plot(x, res_seq, label="Sequential Results")
    plt.plot(x, res_par, label="Parallel Results")
    plt.plot(x, pi, label="Expected Result")
    plt.xlabel("Value of N")
    plt.ylabel("calculated value of PI")
    plt.legend()
    plt.gca().set_ylim([3.14157, 3.14172])

    # show both plots
    plt.show()


def get_data(file_name: str) -> Tuple[list, list, list, list, list]:
    x = list()
    y_seq = list()
    y_par = list()
    res_seq = list()
    res_par = list()
    with open(file_name, "r") as input_file:
        for line in input_file:
            data = line.split(",")
            x.append(int(data[0]))
            y_seq.append(int(data[1]))
            y_par.append(int(data[2]))
            res_seq.append(float(data[3]))
            res_par.append(float(data[4]))
    return x, y_seq, y_par, res_seq, res_par


if __name__ == '__main__':
    main()
