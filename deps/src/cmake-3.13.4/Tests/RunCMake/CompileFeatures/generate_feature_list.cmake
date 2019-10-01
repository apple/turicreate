
enable_language(C)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/c_features.txt"
  "${CMAKE_C_COMPILE_FEATURES}"
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cxx_features.txt"
  "${CMAKE_CXX_COMPILE_FEATURES}"
)

if(DEFINED CMAKE_C_STANDARD_DEFAULT)
  set(c_standard_default_code "set(C_STANDARD_DEFAULT \"${CMAKE_C_STANDARD_DEFAULT}\")\n")
else()
  set(c_standard_default_code "unset(C_STANDARD_DEFAULT)\n")
endif()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/c_standard_default.cmake" "${c_standard_default_code}")

if(DEFINED CMAKE_CXX_STANDARD_DEFAULT)
  set(cxx_standard_default_code "set(CXX_STANDARD_DEFAULT \"${CMAKE_CXX_STANDARD_DEFAULT}\")\n")
else()
  set(cxx_standard_default_code "unset(CXX_STANDARD_DEFAULT)\n")
endif()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cxx_standard_default.cmake" "${cxx_standard_default_code}")

foreach(standard 98 11)
  set(CXX${standard}_FLAG NOTFOUND)
  if (DEFINED CMAKE_CXX${standard}_STANDARD_COMPILE_OPTION)
    set(CXX${standard}_FLAG ${CMAKE_CXX${standard}_STANDARD_COMPILE_OPTION})
  endif()

  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cxx${standard}_flag.txt"
    "${CXX${standard}_FLAG}"
  )
  set(CXX${standard}EXT_FLAG NOTFOUND)
  if (DEFINED CMAKE_CXX${standard}_EXTENSION_COMPILE_OPTION)
    set(CXX${standard}EXT_FLAG ${CMAKE_CXX${standard}_EXTENSION_COMPILE_OPTION})
  endif()

  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cxx${standard}ext_flag.txt"
    "${CXX${standard}EXT_FLAG}"
  )
endforeach()
