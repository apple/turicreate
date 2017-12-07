enable_language(C)
add_library(foo empty.c empty2.c)
target_sources(foo PRIVATE empty3.c)
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/foo.txt
     CONTENT "$<TARGET_PROPERTY:foo,SOURCES>")
