add_library(myobj OBJECT ${CMAKE_BINARY_DIR}/depends_obj.c)
add_library(mylib STATIC $<TARGET_OBJECTS:myobj> depends_lib.c)
add_executable(myexe depends_main.c)
target_link_libraries(myexe mylib)

enable_testing()
add_test(NAME myexe COMMAND $<TARGET_FILE:myexe>)
