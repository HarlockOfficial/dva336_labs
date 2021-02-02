#include <iostream>
#include <chrono>
#include <omp.h>
#include <x86intrin.h>
#include <limits>

#define NUM_THREADS 4
#define ELEMENTS_IN_VECTOR 2

double approximate_pi(unsigned long long int N){
    double return_value=0;
    #pragma omp parallel num_threads(NUM_THREADS)
    {
        __m128d dx = _mm_set1_pd(1.0 / (double) N);
        __m128d base = _mm_set_pd(0.0, 1.0);
        __m128d sum = _mm_set1_pd(0.0);
        __m128d one = _mm_set1_pd(1.0);
        __m128d elements_in_vector = _mm_set1_pd((double)ELEMENTS_IN_VECTOR);

        #pragma omp for
        for (long long unsigned int p = 0; p < N / ELEMENTS_IN_VECTOR; ++p) {
            __m128d i = _mm_add_pd(base, _mm_mul_pd(elements_in_vector, _mm_set1_pd((double)p)));
            __m128d tmp_results = _mm_mul_pd(i, dx);
            tmp_results = _mm_mul_pd(tmp_results, tmp_results);
            tmp_results = _mm_sub_pd(one, tmp_results);
            tmp_results = _mm_sqrt_pd(tmp_results);
            tmp_results = _mm_mul_pd(dx, tmp_results);
            sum = _mm_add_pd(sum, tmp_results);
        }
        sum = _mm_add_pd(sum, _mm_move_sd(sum, sum));
        double tmp = _mm_cvtsd_f64(sum);
        #pragma omp atomic
        return_value += tmp;
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
