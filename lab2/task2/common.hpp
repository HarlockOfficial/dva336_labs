#pragma once

#include <mpi.h>
#include <stdio.h>
#include <unistd.h> //usleep
//#define NDEBUG //uncomment to disable assert
#include <assert.h>

#define DIV(n,m) ((n)/(m)) //truncated division between two integers
#define CEILDIV(n,m) DIV((n)+(m)-1,m) //roundup division between two integers

#define TAG 1234
#define STREAM_ELEMENT_SIZE 10
#define STREAM_LENGTH 100

template <int N> struct streamelement_t {
	/*
	Implementation of a stream element of generic size. 
	The last entry of A is used to test and set the termination guard that is propagated through the 
	modules of the graph when the stream is ended (the initial value is zero 
	which stands for 'not terminated'). 
	The functions send() and  recv() implement the communication with the process passed as parameter. 
	The collecting process for farm and map, however, must be able to receive from any worker in a 
	nondeterministic way. To this end, you can use recv_any() that receives from any process and returns 
	the rank of the sender.
	*/
	streamelement_t() { A[N] = 0; }
	bool is_terminated() const { return A[N]==1; };
	void set_terminated() { A[N] = 1; }
	void printme(int idx) { 
		//prints values of array A
		printf("%3d) [", idx); 
		for(int i=0; i<N; ++i) 
			printf(" %d", A[i]); 
		printf(" ]\n"); 
	}
	int& operator [] (int i) { assert(i>=0 && i<N); return A[i]; }
	void send(int dst) { MPI_Ssend(A, N+1, MPI_INT, dst, TAG, MPI_COMM_WORLD); }
	void recv(int src) { MPI_Recv(A, N+1, MPI_INT, src, TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE); }
	int  recv_any() { 
		MPI_Recv(A, N+1, MPI_INT, MPI_ANY_SOURCE, TAG, MPI_COMM_WORLD, &status); 
		return status.MPI_SOURCE; 
	}
private:
	int A[N+1];
	MPI_Status status;
};

using streamelement = streamelement_t<STREAM_ELEMENT_SIZE>;

inline static void sleep_milliseconds(int ms) { assert(ms>=0 && ms<1000); usleep(ms*1000); }

#define T_f0 4
#define T_f1 8
#define T_g1 12
#define T_f2 1

static int f0(int n) { sleep_milliseconds(T_f0); return n+1; }
static int f1(int n) { sleep_milliseconds(T_f1); return n*2; }
static int g1(int n) { sleep_milliseconds(T_g1); return n/2; }
static int f2(int n) { sleep_milliseconds(T_f2); return n*n; }

static void in(const int dst) {
	streamelement x;
	for(int i=0; i<STREAM_LENGTH; ++i) {
		for(int j=0; j<STREAM_ELEMENT_SIZE; ++j)
			x[j] = f0(j);	//x[j]=j+1 4ms delay
		x.send(dst);
	}
	x.set_terminated();
	x.send(dst);
}

static void out(const int src) {
	streamelement x;
	int msg_counter = 0;
	while(true) {
		x.recv(src);
		if(x.is_terminated())
			return;
		for(int j=0; j<STREAM_ELEMENT_SIZE; ++j)
			x[j] = f2(x[j]);	//x[j]*=x[j] 1ms delay
		x.printme(++msg_counter);
	}
}

static double get_time(MPI_Comm comm) {
	MPI_Barrier(comm);
	return MPI_Wtime();
	MPI_Barrier(comm);
}
