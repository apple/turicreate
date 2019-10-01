# A dummy checker implementation that does not write the requested output file
# so it triggers an error for every checker.

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ret0.c.in" "int main(){return 0;}\n")

configure_file(
  "${CMAKE_CURRENT_BINARY_DIR}/ret0.c.in"
  "${CMAKE_CURRENT_BINARY_DIR}/ret0.c"
  )

foreach(_pseudo IN ITEMS valgrind purify BC)
  add_executable(pseudonl_${_pseudo} "${CMAKE_CURRENT_BINARY_DIR}/ret0.c")
  set_target_properties(pseudonl_${_pseudo} PROPERTIES OUTPUT_NAME ${_pseudo})
endforeach()
