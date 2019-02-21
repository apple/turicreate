add_library(outlib outlib.c)

add_executable(outexe outexe.c)
target_link_libraries(outexe subdir)
