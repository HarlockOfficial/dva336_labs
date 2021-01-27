#include <iostream>
#include <omp.h>
#include <xmmintrin.h>

#define ALIGNMENT 16 //using sse
#define NUM_THREADS 4
#define N 65536

float approximate_pi(){
    const float dx = 1.0/(float)N;
    alignas(ALIGNMENT) float out[N/4];

    #pragma omp parallel for num_threads(NUM_THREADS)
    for(int p=0;p<N/4;++p) {
        alignas(ALIGNMENT) float x[4];
        #pragma omp parallel for num_threads(NUM_THREADS)
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
    for(int i = 0;i<N/4;++i){
        return_value+=out[i];
    }
    return return_value*4;
}

int main(){
    double start, result, parallel_time;
    start = omp_get_wtime();
    result = approximate_pi();
    parallel_time = omp_get_wtime() - start;
    std::cout<<"result: "<<result<<" in "<<parallel_time<<" ms\n";
    return 0;
}
