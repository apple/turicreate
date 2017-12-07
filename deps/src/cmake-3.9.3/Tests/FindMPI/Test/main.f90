program mpi_test
  include 'mpif.h'
  integer ierror

  call MPI_INIT(ierror)
  call MPI_FINALIZE(ierror)
end program mpi_test
