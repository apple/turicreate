

if(UNIX  AND  "${CMAKE_GENERATOR}" MATCHES "Makefile" AND
    NOT CMake_TEST_NO_FindPackageModeMakefileTest)

  # Test whether the make is GNU make, and only add the test in this case,
  # since the configured makefile in this test uses $(shell ...), which
  # is AFAIK a GNU make extension. Alex
  execute_process(COMMAND ${CMAKE_MAKE_PROGRAM} -v
                  OUTPUT_VARIABLE makeVersionOutput
                  ERROR_QUIET
                  TIMEOUT 10)
  string(TOUPPER "${makeVersionOutput}" MAKE_VERSION_OUTPUT)
  if("${MAKE_VERSION_OUTPUT}" MATCHES "GNU MAKE")

    # build a library which we can search during the test
    add_library(foo STATIC foo.cpp)

    # configure a FindFoo.cmake so it knows where the library can be found
    configure_file(FindFoo.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/FindFoo.cmake @ONLY)

    # now set up the test:
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/cmakeExecutable.mk"
      CONTENT "CMAKE = \"$<TARGET_FILE:cmake>\"\n"
    )
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Makefile.in ${CMAKE_CURRENT_BINARY_DIR}/ConfMakefile @ONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ${CMAKE_CURRENT_BINARY_DIR}/main.cpp COPYONLY)

    add_test(FindPackageModeMakefileTest ${CMAKE_MAKE_PROGRAM} -f ${CMAKE_CURRENT_BINARY_DIR}/ConfMakefile )

  endif()

endif()
