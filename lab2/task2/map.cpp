#include "common.hpp"

void split(int num_worker, int in){
    streamelement x;
    while(true) {
		x.recv(in);
		if(x.is_terminated()) {
			for(int i=0;i<num_worker;i++)
				x.send(2+i);
			return;
		}
		for(int i=0;i<num_worker;i++)
			x.send(2+i);
    }
}

//process 2 ... num_workers-1
void M(int splitter_id, int collector_id, int start, int end) {
	streamelement x;
	while(true) {
		x.recv(splitter_id);
		if(x.is_terminated()) {
			if(start==0)
				x.send(collector_id);
			return;
		}
		for(int j=start; j<end; ++j)
			x[j] = g1(f1(x[j]));
		x.send(collector_id);
	}
}

void collect(int num_worker, int step, int out){     //process 2+num_workers
    streamelement x;
	streamelement z;
    while(true){
		for(int i=0;i<num_worker;i++){
			z.recv(i+2);
			if(z.is_terminated()) {
				z.send(out);
				return;
			}
			for(int j=i*step;j<i*step+step && j<STREAM_ELEMENT_SIZE;j++){
				x[j]=z[j];
			}
		}
        x.send(out);
    }
}

constexpr int required_comm_size = 9;

int main(int argc, char** argv) {
	MPI_Init(&argc, &argv);
	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	int num_workers=5;
	int step=STREAM_ELEMENT_SIZE/num_workers;
	if(size==required_comm_size) {
		double start = get_time(MPI_COMM_WORLD);
		if(rank==0) in(1);
        if(rank==1) split(num_workers,0);
		for(int i=0;i<num_workers;i++){
			if(rank==2+i) M(1,7,step*i,step*i+step);
		}
        if(rank==2+num_workers) collect(num_workers,step,3+num_workers);
		if(rank==3+num_workers) out(2+num_workers);
		double end = get_time(MPI_COMM_WORLD);
		if(rank==0) sleep_milliseconds(250),printf("elapsed time = %.2f seconds\n", end-start);
	} else {
		if(rank==0) printf("run with option '-n %d'\n", required_comm_size);
	}
	MPI_Finalize();
	return 0;
}
