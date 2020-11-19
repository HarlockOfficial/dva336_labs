#include <mpi.h>
#include <unistd.h>
#include <iostream>

#define COMMUNICATION_TURNS 2

//complexity
//https://stackoverflow.com/questions/10625643/mpi-communication-complexity

void My_Barrier(MPI_Comm comm){
   int rank, size, message;
   MPI_Comm_size(MPI_COMM_WORLD, &size);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   
   const int tag = 3, count = 1;
   const int receiver = (rank+1)%size;
   const int sender = (rank>0?rank-1:size-1);

   if(rank == 0){
      message = 10;
      MPI_Send(&message, count, MPI_INT, receiver, tag, MPI_COMM_WORLD);
      //std::cout<<rank<<" sended: "<<message<<" to: "<<receiver<<"\n";
   }
   for(int i=0;i<COMMUNICATION_TURNS; i++){
      MPI_Recv(&message, count, MPI_INT, sender, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      //std::cout<<rank<<" received: "<<message<<" from: "<<sender<<"\n";
      message +=1;
      MPI_Send(&message, count, MPI_INT, receiver, tag, MPI_COMM_WORLD);
      //std::cout<<rank<<" sended: "<<message<<" to: "<<receiver<<"\n";
   }
}

int main(int argc, char **argv) {
   MPI_Init(&argc, &argv);
   int rank, size;
   MPI_Comm_size(MPI_COMM_WORLD, &size);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   std::cout<<"Processor with rank "<<rank<<" out of "<<size<<" processors : barrier reached!\n";
   sleep(1);
   My_Barrier(MPI_COMM_WORLD);
   std::cout<<"Processor with rank "<<rank<<" out of "<<size<<" processors : barrier passed!\n";
   MPI_Finalize();
   return 0;
}
