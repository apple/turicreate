add_custom_target(drive)
add_custom_command(TARGET drive PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo $<COMPILE_LANGUAGE>
)
