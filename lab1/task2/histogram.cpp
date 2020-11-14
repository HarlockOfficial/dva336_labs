#include <omp.h>
#include <iostream>
#include <cstdlib>

#define NTHREADS 4 // this should be equal to how many cores are available
#define NBUCKETS 67

void seq_histogram(int* array, int n, int histogram[NBUCKETS]) {
	//reset buckets
	for (int i = 0; i < NBUCKETS; ++i)
		histogram[i] = 0;
	//populate buckets
	for (int i = 0; i < n; ++i)
		++histogram[array[i]];
}

void par_histogram(int* array, int n, int histogram[NBUCKETS]) {	//not faster
	//parallel reset of buckets
	#pragma omp parallel for
	for(int i=0; i < NBUCKETS; i++)
		histogram[i] = 0;
	//parallel population of buckets
	#pragma omp parallel for
	for(int i=0; i < n; i++)
		#pragma omp atomic
		++histogram[array[i]];
}

#define ROUNDUPDIV(n,d) (((n)+(d)-1)/(d)) //round-up the result of the division n/d where n and d are integer values

void par_histogram_aligned(int* array, int n, int histogram[NBUCKETS]) {	//dont know what to do
	/*
	YOUR IMPLEMENTATION GOES HERE
	*/
}

bool check(int n, int histogram[NBUCKETS])
{
	int sum = 0;
	for (int i = 0; i < NBUCKETS; ++i)
		sum += histogram[i];
	return sum == n;
}

int main() {
	//init input
	int n = 250*1000*1000;
	int *arr = new int[n];
	for (int i = 0; i < n; ++i)
		arr[i] = rand()%NBUCKETS;

	int histogram[NBUCKETS];

	std::cout<<"number of items = "<<n<<"\n";
	std::cout<<"number of buckets = "<<NBUCKETS<<"\n";
	std::cout<<"number of threads = "<<NTHREADS<<"\n";

	double ts = omp_get_wtime();
	seq_histogram(arr, n, histogram);
	ts = omp_get_wtime() - ts;

	std::cout<<"seq_histogram, elapsed time = "<<ts<<" seconds, check passed = "<<(check(n,histogram)?'Y':'N')<<"\n";

	double tp = omp_get_wtime();
	par_histogram(arr, n, histogram);
	tp = omp_get_wtime() - tp;
	std::cout<<"par_histogram, elapsed time = "<<tp<<" seconds ("<<ts/tp<<"x speedup), check passed = "<<(check(n, histogram) ? 'Y' : 'N')<<"\n";

	double tpa = omp_get_wtime();
	par_histogram_aligned(arr, n, histogram);
	tpa = omp_get_wtime() - tpa;
	std::cout<<"par_histogram_aligned, elapsed time = "<<tpa<<" seconds ("<<ts/tpa<<"x speedup), check passed = "<<(check(n, histogram) ? 'Y' : 'N')<<"\n";

	delete[] arr;

	return 0;
}

