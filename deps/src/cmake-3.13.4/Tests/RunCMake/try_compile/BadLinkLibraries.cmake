add_custom_target(not_a_library)
try_compile(RESULT ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src.c
  LINK_LIBRARIES not_a_library)
