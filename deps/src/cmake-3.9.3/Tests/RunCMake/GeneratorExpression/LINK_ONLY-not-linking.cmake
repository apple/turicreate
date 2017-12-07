add_custom_target(Custom ALL COMMAND ${CMAKE_COMMAND} -E echo $<LINK_ONLY:something>)
