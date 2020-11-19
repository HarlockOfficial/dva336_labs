#include <mpi.h>
#include <iostream>

int main(int argc, char** argv) {
   MPI_Init(&argc, &argv);
   int rank, size;
   char message;
   MPI_Comm_size(MPI_COMM_WORLD, &size);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   const int tag=16, sender=0, receiver=1, count=1;
   if (rank==0) {
      message = 'a';
      MPI_Send(&message, count, MPI_BYTE, receiver, tag, MPI_COMM_WORLD);
   }
   if (rank==1) {
      MPI_Recv(&message, count, MPI_BYTE, sender, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      std::cout<<"Process 1 received "<<message<<" from process 0\n";
   }
   MPI_Finalize();
}