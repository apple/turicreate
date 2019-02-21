add_custom_target(not_a_library)
try_run(RUN_RESULT COMPILE_RESULT
  ${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp ${CMAKE_CURRENT_SOURCE_DIR}/src.c
  LINK_LIBRARIES not_a_library)
