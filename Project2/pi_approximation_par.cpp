#include <iostream>
#include <chrono>
#include <omp.h>
#include <xmmintrin.h>

#define ALIGNMENT 16 //using sse
#define NUM_THREADS 4

float approximate_pi(unsigned long long int N){
    const float dx = 1.0/(float)N;
    alignas(ALIGNMENT) float out[N/4];

    #pragma omp parallel for num_threads(NUM_THREADS)
    for(int p=0;p<N/4;++p) {
        alignas(ALIGNMENT) float x[4];
        for (int i = 4*p; i < 4*p+4; ++i) {
            x[i-4*p] = 1 - i * dx * i * dx;
        }
        __m128 f_xi = _mm_sqrt_ps(_mm_load_ps(x));
        __m128 tmp_results = _mm_mul_ps(_mm_set1_ps(dx), f_xi);

        //now do horizontal reduce to obtain the final value
        tmp_results = _mm_add_ps(tmp_results, _mm_movehl_ps(tmp_results, tmp_results));
        tmp_results = _mm_add_ss(tmp_results, _mm_shuffle_ps(tmp_results, tmp_results, _MM_SHUFFLE(0, 0, 0, 1)));
        out[p] = _mm_cvtss_f32(tmp_results);
    }
    float return_value = 0.0f;
    for (int i = 0; i < N / 4; ++i) {
        return_value += out[i];
    }
    return return_value*4;
}

int main(int argc, char** argv){
    if(argc <= 1){
        std::cout<<"Usage: "<<argv[0]<<" [number of nodes]\n";
        return -1;
    }
    std::cout<<strtoull(argv[1], NULL, 10);
    double result;
    auto funct_start = std::chrono::steady_clock::now();
    result = approximate_pi(strtoull(argv[1], NULL, 10));
    auto funct_end = std::chrono::steady_clock::now();
    std::cout<<"result: "<<result<<" in "<<std::chrono::duration_cast<std::chrono::milliseconds>(funct_end - funct_start).count()<<"\n";
    return 0;
}
