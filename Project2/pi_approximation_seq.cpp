#include <iostream>
#include <chrono>
#include <cmath>

float approximate_pi(unsigned long long int N) {
    float sum = 0;
    for (long long unsigned int i = 0;i < N;i++) {
        sum += ((float)((float)sqrt(1.0 - ((((float)i) / ((float)N)) * (((float)i) / ((float)N))))) / ((float)N));
    }
    return 4.0 * sum;
}

int main(int argc, char** argv){
    if(argc <= 1){
        std::cout<<"Usage: "<<argv[0]<<" [number of nodes]\n";
        return -1;
    }
    float result;
    auto funct_start = std::chrono::steady_clock::now();
    result = approximate_pi(strtoull(argv[1], NULL, 10));
    auto funct_end = std::chrono::steady_clock::now();
    std::cout<<"result: "<<result<<" in "<<std::chrono::duration_cast<std::chrono::milliseconds>(funct_end - funct_start).count()<<"\n";
    return 0;
}
