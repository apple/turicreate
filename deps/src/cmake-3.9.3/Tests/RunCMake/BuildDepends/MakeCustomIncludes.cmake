add_custom_command(
  OUTPUT output.cxx
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/MakeCustomIncludes.cxx output.cxx
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/MakeCustomIncludes.cxx
  IMPLICIT_DEPENDS CXX ${CMAKE_CURRENT_SOURCE_DIR}/MakeCustomIncludes.cxx)
add_custom_target(generate ALL DEPENDS output.cxx)
set_property(TARGET generate PROPERTY INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR})

file(GENERATE OUTPUT check-$<LOWER_CASE:$<CONFIG>>.cmake CONTENT "
set(check_pairs
  \"${CMAKE_CURRENT_BINARY_DIR}/output.cxx|${CMAKE_CURRENT_BINARY_DIR}/MakeCustomIncludes.h\"
  )
")
