#include <iostream>
#include <omp.h>
#include <cmath>

/*double approximate_pi_sequential_v1(long int N){    //original
    double sum = 0;
    double delta_x = 1.0/N;
    for(int i=0;i<N;i++){
        double x_i = i*delta_x;
        sum+=(delta_x * sqrt(1-(x_i*x_i)));
    }
    return 4*sum;
}*/

double approximate_pi_sequential(long int N){   //reduced variables
    double sum = 0;
    for(int i=0;i<N;i++){
        sum+=((double)((double)sqrt(1.0-((((double)i)/((double)N))*(((double)i)/((double)N)))))/((double)N));
    }
    return 4*sum;
}


double approximate_pi_parallel(long int N){
    double sum = 0;
    #pragma omp parallel for 
    for(int i=0;i<N;i++){
        #pragma omp atomic
        sum+=((double)((double)sqrt(1.0-((((double)i)/((double)N))*(((double)i)/((double)N)))))/((double)N));
    }
    return 4*sum;
}

void call_parallel(int thread_number, long int N){
    double result;

    std::cout<<"start parallel\n";
    double start=omp_get_wtime();
    #pragma omp parallel num_threads(thread_number)
    #pragma omp single
    result = approximate_pi_parallel(N);
    double parallel_time = omp_get_wtime()-start;
    std::cout<<"ended parallel\n";
    
    std::cout<<"Parallel result ("<<thread_number<<" thread/s): "<<result<<"\n";
    std::cout<<"Parallel Work ("<<thread_number<<" thread/s) took: "<<parallel_time<<"\n";
}

int main()
{
    long int N = 100000000000;
    
    int thread_number = 8;    
    call_parallel(thread_number,N);

    thread_number=4;    
    call_parallel(thread_number,N);
    

    thread_number=3;
    call_parallel(thread_number, N);

    thread_number=2;
    call_parallel(thread_number,N);

    thread_number=1;
    call_parallel(thread_number,N);
    
    std::cout<<"start sequential\n";
    double start = omp_get_wtime();
    double result = approximate_pi_sequential(N);
    double sequential_time = omp_get_wtime()-start;
    std::cout<<"ended sequential\n";

    std::cout<<"Sequential result: "<<result<<"\n";
    std::cout<<"Sequential Work took: "<<sequential_time<<"\n";

    return 0;
}
