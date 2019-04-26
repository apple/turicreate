#include <mpi.h>

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);
  MPI_Finalize();
}
