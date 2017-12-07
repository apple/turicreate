include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMake_BINARY_DIR}/Source
  ${CMake_SOURCE_DIR}/Source
  )

set(CMakeLib_TESTS
  testGeneratedFileStream
  testRST
  testSystemTools
  testUTF8
  testXMLParser
  testXMLSafe
  testFindPackageCommand
  )

set(testRST_ARGS ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
  list(APPEND CMakeLib_TESTS
    testVisualStudioSlnParser
    )
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/testVisualStudioSlnParser.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/testVisualStudioSlnParser.h @ONLY)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/testXMLParser.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/testXMLParser.h @ONLY)

create_test_sourcelist(CMakeLib_TEST_SRCS CMakeLibTests.cxx ${CMakeLib_TESTS})
add_executable(CMakeLibTests ${CMakeLib_TEST_SRCS})
target_link_libraries(CMakeLibTests CMakeLib)

add_executable(testEncoding testEncoding.cxx)
target_link_libraries(testEncoding cmsys)

foreach(test ${CMakeLib_TESTS})
  add_test(CMakeLib.${test} CMakeLibTests ${test} ${${test}_ARGS})
endforeach()

if(TEST_CompileCommandOutput)
  add_executable(runcompilecommands run_compile_commands.cxx)
  target_link_libraries(runcompilecommands CMakeLib)
endif()

add_subdirectory(PseudoMemcheck)
