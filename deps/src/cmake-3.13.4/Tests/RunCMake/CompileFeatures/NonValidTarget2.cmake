
set(genexvar $<COMPILE_FEATURES:cxx_final>)

add_custom_target(copy_target
  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/empty.cpp" "${CMAKE_CURRENT_BINARY_DIR}/copied_file${genexvar}.txt"
)
set_property(SOURCE "${CMAKE_CURRENT_BINARY_DIR}/copied_file0.cpp" PROPERTY GENERATED 1)
set_property(SOURCE "${CMAKE_CURRENT_BINARY_DIR}/copied_file1.cpp" PROPERTY GENERATED 1)

add_library(empty "${CMAKE_CURRENT_BINARY_DIR}/copied_file${genexvar}.cpp")
