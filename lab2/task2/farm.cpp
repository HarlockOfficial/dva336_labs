#include "common.hpp"

void split(int tag, int num_of_workers, int in){
    int worker;
    streamelement x;
    while(true){
        x.recv(in);
		if(x.is_terminated()){
			for(int i=0;i<num_of_workers;i++){
				MPI_Recv(&worker, 1, MPI_INT, i+2, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				x.send(i+2);
			}
			return;
		}
        MPI_Recv(&worker,1,MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        x.send(worker);
    }
}
void M(int tag, int rank, int splitter_thread_id, int collector_thread_id) {
	streamelement x;
    while(true) {
        MPI_Send(&rank, 1, MPI_INT, splitter_thread_id, tag, MPI_COMM_WORLD);
		x.recv(splitter_thread_id);
        if(x.is_terminated()) {
			if(rank==collector_thread_id-1)
				x.send(collector_thread_id);
			return;
		}
		for(int j=0; j<STREAM_ELEMENT_SIZE; ++j)
			x[j] = g1(f1(x[j]));
		x.send(collector_thread_id);
	}
}

void collect(int out){
    streamelement x;
    while(true){
        x.recv_any();
        x.send(out);
		if(x.is_terminated()){
			return;
		}
    }
}

constexpr int required_comm_size = 9;

int main(int argc, char** argv) {
	MPI_Init(&argc, &argv);
	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	int num_workers = 5, tag = 7;
	if(size==required_comm_size) {
		double start = get_time(MPI_COMM_WORLD);
		if(rank==0) in(1);
        if(rank==1) split(tag, num_workers, 0);
		for(int i=0;i<num_workers;i++){
			if(rank==2+i) M(tag,rank,1, 2+num_workers);
		}
        if(rank==2+num_workers) collect(3+num_workers); 
		if(rank==3+num_workers) out(2+num_workers);
		double end = get_time(MPI_COMM_WORLD);
		if(rank==0) sleep_milliseconds(250),printf("elapsed time = %.2f seconds\n", end-start);
	} else {
		if(rank==0) printf("run with option '-n %d'\n", required_comm_size);
	}
	MPI_Finalize();
	return 0;
}
