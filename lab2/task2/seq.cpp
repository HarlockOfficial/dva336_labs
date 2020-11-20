#include "common.hpp"

void M() {
	streamelement x;
	while(true) {
		x.recv(0);
		if(x.is_terminated()) {
			x.send(2);
			return;
		}
		for(int j=0; j<STREAM_ELEMENT_SIZE; ++j)
			x[j] = g1(f1(x[j]));
		x.send(2);
	}
}

constexpr int required_comm_size = 3;

int main(int argc, char** argv) {
	MPI_Init(&argc, &argv);
	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	if(size==required_comm_size) {
		double start = get_time(MPI_COMM_WORLD);
		if(rank==0) in(1);
		if(rank==1) M();
		if(rank==2) out(1);
		double end = get_time(MPI_COMM_WORLD);
		if(rank==0) sleep_milliseconds(250),printf("elapsed time = %.2f seconds\n", end-start);
	} else {
		if(rank==0) printf("run with option '-n %d'\n", required_comm_size);
	}
	MPI_Finalize();
	return 0;
}
