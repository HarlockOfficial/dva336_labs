#include <iostream>
#include <chrono>
#include <omp.h>
#include <x86intrin.h>
#include <limits>

#define ALIGNMENT 16 //using sse
#define NUM_THREADS 4
#define ELEMENTS_IN_VECTOR 2

double approximate_pi(unsigned long long int N){
    const double dx = 1.0/(double)N;
    double return_value=0;

    #pragma omp parallel for num_threads(NUM_THREADS)
    for(long long unsigned int p=0;p<N/ELEMENTS_IN_VECTOR;++p) {
        __m128d f_xi = _mm_sqrt_pd(_mm_set_pd(1 - (ELEMENTS_IN_VECTOR * p * dx * ELEMENTS_IN_VECTOR * p * dx),
                                              1 - (ELEMENTS_IN_VECTOR * p + 1) * dx * (ELEMENTS_IN_VECTOR * p + 1) * dx));
        __m128d tmp_results = _mm_mul_pd(_mm_set1_pd(dx), f_xi);

        //now do horizontal reduce to obtain the final value
        tmp_results = _mm_add_pd(tmp_results, _mm_move_sd(tmp_results, tmp_results));
        double tmp = _mm_cvtsd_f64(tmp_results);
        #pragma omp atomic
        return_value+=tmp;
    }
    return return_value*4;
}

int main(int argc, char** argv){
    if(argc <= 1){
        std::cout<<"Usage: "<<argv[0]<<" [number of nodes]\n";
        return -1;
    }
    double result;
    auto funct_start = std::chrono::steady_clock::now();
    result = approximate_pi(strtoull(argv[1], NULL, 10));
    auto funct_end = std::chrono::steady_clock::now();
    std::cout.precision(std::numeric_limits<double>::max_digits10);
    std::cout<<"result: "<<result<<" in "<<std::chrono::duration_cast<std::chrono::milliseconds>(funct_end - funct_start).count()<<"\n";
    return 0;
}
