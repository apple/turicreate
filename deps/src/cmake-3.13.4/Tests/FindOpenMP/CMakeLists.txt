foreach(c C CXX Fortran)
  if(CMake_TEST_FindOpenMP_${c})
    set(CMake_TEST_FindOpenMP_FLAG_${c} 1)
  else()
    set(CMake_TEST_FindOpenMP_FLAG_${c} 0)
  endif()
endforeach()

add_test(NAME FindOpenMP.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindOpenMP/Test"
  "${CMake_BINARY_DIR}/Tests/FindOpenMP/Test"
  ${build_generator_args}
  --build-project TestFindOpenMP
  --build-options ${build_options}
  -DOpenMP_TEST_C=${CMake_TEST_FindOpenMP_FLAG_C}
  -DOpenMP_TEST_CXX=${CMake_TEST_FindOpenMP_FLAG_CXX}
  -DOpenMP_TEST_Fortran=${CMake_TEST_FindOpenMP_FLAG_Fortran}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
