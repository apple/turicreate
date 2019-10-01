
include(CTest)
enable_testing()

add_test(NAME dummy COMMAND ${CMAKE_COMMAND} -E echo $<COMPILE_LANGUAGE>)
