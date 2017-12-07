enable_language(C)
add_library(foo empty.c)
add_custom_target(drive COMMAND ${CMAKE_COMMAND} -E echo $<TARGET_PROPERTY:foo,LOCATION>)
