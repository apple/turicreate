add_subdirectory(TargetNotInDir)
add_custom_command(TARGET TargetNotInDir COMMAND ${CMAKE_COMMAND} -E echo tada)
