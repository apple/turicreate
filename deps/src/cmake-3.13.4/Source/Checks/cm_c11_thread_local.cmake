set(CMake_C11_THREAD_LOCAL_BROKEN 0)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_C11_STANDARD_COMPILE_OPTION)
  if(NOT DEFINED CMake_C11_THREAD_LOCAL_WORKS)
    message(STATUS "Checking if compiler supports C11 _Thread_local")
    try_compile(CMake_C11_THREAD_LOCAL_WORKS
      ${CMAKE_CURRENT_BINARY_DIR}
      ${CMAKE_CURRENT_LIST_DIR}/cm_c11_thread_local.c
      CMAKE_FLAGS -DCMAKE_C_STANDARD=11
      OUTPUT_VARIABLE OUTPUT
      )
    if(CMake_C11_THREAD_LOCAL_WORKS AND "${OUTPUT}" MATCHES "error: expected '=', ',', ';', 'asm' or '__attribute__' before 'int'")
      set_property(CACHE CMake_C11_THREAD_LOCAL_WORKS PROPERTY VALUE 0)
    endif()
    if(CMake_C11_THREAD_LOCAL_WORKS)
      message(STATUS "Checking if compiler supports C11 _Thread_local - yes")
      file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
        "Determining if compiler supports C11 _Thread_local passed with the following output:\n"
        "${OUTPUT}\n"
        "\n"
        )
    else()
      message(STATUS "Checking if compiler supports C11 _Thread_local - no")
      file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "Determining if compiler supports C11 _Thread_local failed with the following output:\n"
        "${OUTPUT}\n"
        "\n"
        )
    endif()
  endif()
  if(NOT CMake_C11_THREAD_LOCAL_WORKS)
    set(CMake_C11_THREAD_LOCAL_BROKEN 1)
  endif()
endif()
