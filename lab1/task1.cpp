#include <iostream>
#include <omp.h>
#include <cmath>


double approximate_pi_sequential(long int N) {   //reduced variables
    double sum = 0;
    for (int i = 0;i < N;i++) {
        sum += ((double)((double)sqrt(1.0 - ((((double)i) / ((double)N)) * (((double)i) / ((double)N))))) / ((double)N));
    }
    return 4.0 * sum;
}


double approximate_pi_parallel(long int N,int n) {
    double sum = 0; // shared
#pragma omp parallel num_threads(n)
    {
        double partialsum = 0; // private
#pragma omp for nowait
        for (int i = 0;i < N;i++) {
            partialsum += ((double)((double)sqrt(1.0 - ((((double)i) / ((double)N)) * (((double)i) / ((double)N))))) / ((double)N));
        }
#pragma omp atomic
        sum += partialsum; // merge
    }
    return sum * 4.0;
}




int main()
{
    long int N = 100000000000;
    double start, result, parallel_time;

    for (int i = 2; i < 9; )
    {
        std::cout << "Start parallel : " << i << " threads\n";
        start = omp_get_wtime();
        result = approximate_pi_parallel(N, i);
        parallel_time = omp_get_wtime() - start;
        std::cout << "Parallel result " << result << "\n";
        std::cout << "Parallel Work took: " << parallel_time << "\n\n";
        i = i + 2;

    }




    std::cout << "Start sequential\n";
     start = omp_get_wtime();
    result = approximate_pi_sequential(N);
    double sequential_time = omp_get_wtime() - start;
    std::cout << "ended sequential\n";

    std::cout << "Sequential result: " << result << "\n";
    std::cout << "Sequential Work took: " << sequential_time << "\n";

    return 0;
}
