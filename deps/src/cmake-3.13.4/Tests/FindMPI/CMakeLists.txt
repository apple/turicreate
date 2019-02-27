foreach(c C CXX Fortran)
  if(CMake_TEST_FindMPI_${c})
    set(CMake_TEST_FindMPI_FLAG_${c} 1)
  else()
    set(CMake_TEST_FindMPI_FLAG_${c} 0)
  endif()
endforeach()

add_test(NAME FindMPI.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindMPI/Test"
  "${CMake_BINARY_DIR}/Tests/FindMPI/Test"
  ${build_generator_args}
  --build-project TestFindMPI
  --build-options ${build_options}
  -DMPI_TEST_C=${CMake_TEST_FindMPI_FLAG_C}
  -DMPI_TEST_CXX=${CMake_TEST_FindMPI_FLAG_CXX}
  -DMPI_TEST_Fortran=${CMake_TEST_FindMPI_FLAG_Fortran}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
