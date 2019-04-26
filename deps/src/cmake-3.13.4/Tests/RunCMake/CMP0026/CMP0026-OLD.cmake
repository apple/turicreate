enable_language(CXX)

cmake_policy(SET CMP0026 OLD)

set(out ${CMAKE_CURRENT_BINARY_DIR}/out.txt)

add_library(somelib empty.cpp ${out})
get_target_property(_loc somelib LOCATION)

file(WRITE "${out}"
  "source file written by project code after getting target LOCATION\n"
  )
