#include "common.hpp"

void M1() {
	streamelement x;
	while(true) {
		x.recv(0); //receive integer array from thread 0 and stores to A
		if(x.is_terminated()) { //x is terminated after STREAM_LENGTH+1 times
			x.send(2);  //x is sent to thread 2 and not edited
			return;
		}
		for(int j=0; j<STREAM_ELEMENT_SIZE; ++j)
            //multiply by 2 the number litterally x[j] = x[j]*2 with 8ms sleep
            x[j] = f1(x[j]);
		x.send(2);  //x is sent to thread 2 after edit
	}
}

void M2() {
	streamelement x;
	while(true) {
		x.recv(1); //receive integer array from thread 1 and stores to A
		if(x.is_terminated()) { //x is terminated after STREAM_LENGTH+1 times
			x.send(3);  //x is sent to thread 3 and not edited
			return;
		}
		for(int j=0; j<STREAM_ELEMENT_SIZE; ++j)
            //divide by 2 the number x[j] = x[j]/2 with 12ms sleep
            x[j] = g1(x[j]);
		x.send(3);  //x is sent to thread 3 after edit
	}
}

constexpr int required_comm_size = 4;

int main(int argc, char** argv) {
	MPI_Init(&argc, &argv);
	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	if(size==required_comm_size) {
		double start = get_time(MPI_COMM_WORLD);
        //thread 0 sends data to M()
		if(rank==0) in(1);
        //thread 1 is M()
		if(rank==1) M1();
        //thread 2 receives data from M()
		if(rank==2) M2();
		if(rank==3) out(2);
		double end = get_time(MPI_COMM_WORLD);
		//thread 0 prints elapsed time
        if(rank==0) {
            sleep_milliseconds(250);
            printf("elapsed time = %.2f seconds\n", end-start);
        }
	} else {
		if(rank==0) printf("run with option '-n %d'\n", required_comm_size);
	}
	MPI_Finalize();
	return 0;
}
