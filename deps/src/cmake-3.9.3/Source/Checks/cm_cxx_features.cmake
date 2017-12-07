
function(cm_check_cxx_feature name)
  string(TOUPPER ${name} FEATURE)
  if(NOT DEFINED CMake_HAVE_CXX_${FEATURE})
    message(STATUS "Checking if compiler supports C++ ${name}")
    if(CMAKE_CXX_STANDARD)
      set(maybe_cxx_standard -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD})
    else()
      set(maybe_cxx_standard "")
    endif()
    try_compile(CMake_HAVE_CXX_${FEATURE}
      ${CMAKE_CURRENT_BINARY_DIR}
      ${CMAKE_CURRENT_LIST_DIR}/cm_cxx_${name}.cxx
      CMAKE_FLAGS ${maybe_cxx_standard}
      OUTPUT_VARIABLE OUTPUT
      )
    # Filter out MSBuild output that looks like a warning.
    string(REGEX REPLACE " +0 Warning\\(s\\)" "" check_output "${OUTPUT}")
    # If using the feature causes warnings, treat it as broken/unavailable.
    if(check_output MATCHES "[Ww]arning")
      set(CMake_HAVE_CXX_${FEATURE} OFF CACHE INTERNAL "TRY_COMPILE" FORCE)
    endif()
    if(CMake_HAVE_CXX_${FEATURE})
      message(STATUS "Checking if compiler supports C++ ${name} - yes")
      file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
        "Determining if compiler supports C++ ${name} passed with the following output:\n"
        "${OUTPUT}\n"
        "\n"
        )
    else()
      message(STATUS "Checking if compiler supports C++ ${name} - no")
      file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "Determining if compiler supports C++ ${name} failed with the following output:\n"
        "${OUTPUT}\n"
        "\n"
        )
    endif()
  endif()
endfunction()

cm_check_cxx_feature(auto_ptr)
cm_check_cxx_feature(eq_delete)
cm_check_cxx_feature(fallthrough)
if(NOT CMake_HAVE_CXX_FALLTHROUGH)
  cm_check_cxx_feature(gnu_fallthrough)
  if(NOT CMake_HAVE_CXX_GNU_FALLTHROUGH)
    cm_check_cxx_feature(attribute_fallthrough)
  endif()
endif()
cm_check_cxx_feature(make_unique)
if(CMake_HAVE_CXX_MAKE_UNIQUE)
  set(CMake_HAVE_CXX_UNIQUE_PTR 1)
endif()
cm_check_cxx_feature(nullptr)
cm_check_cxx_feature(override)
cm_check_cxx_feature(unique_ptr)
cm_check_cxx_feature(unordered_map)
cm_check_cxx_feature(unordered_set)
