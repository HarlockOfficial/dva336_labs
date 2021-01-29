#include <iostream>
#include <chrono>
#include <cmath>

double approximate_pi(unsigned long long int N) {
    double sum = 0;
    for (long long unsigned int i = 0;i < N;i++) {
        sum += ((double)((double)sqrt(1.0 - ((((double)i) / ((double)N)) * (((double)i) / ((double)N))))) / ((double)N));
    }
    return 4.0 * sum;
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
    std::cout<<"result: "<<result<<" in "<<std::chrono::duration_cast<std::chrono::milliseconds>(funct_end - funct_start).count()<<"\n";
    return 0;
}
