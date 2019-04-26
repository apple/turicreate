
set(genexvar $<COMPILE_FEATURES:cxx_final>)

if (HAVE_FINAL)
  set(expected_result 1)
else()
  set(expected_result 0)
endif()

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/copied_file${expected_result}.cpp"
  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/empty.cpp" "${CMAKE_CURRENT_BINARY_DIR}/copied_file${genexvar}.cpp"
)

add_library(empty "${CMAKE_CURRENT_BINARY_DIR}/copied_file${genexvar}.cpp")
if (HAVE_FINAL)
  target_compile_features(empty PRIVATE cxx_final)
endif()
