#include <mpi.h>
#include <iostream>

int main(int argc, char** argv) {
   MPI_Init(&argc, &argv); // init the MPI environment
   int rank, size;
   MPI_Comm_size(MPI_COMM_WORLD, &size); //get the size of the communicator
   MPI_Comm_rank(MPI_COMM_WORLD, &rank); // get the rank of the process
   std::cout<<"Hello world from processor with rank "<<rank<<" out of "<<size<<" processors\n";
   MPI_Finalize(); // finalize the MPI environment
}