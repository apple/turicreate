
enable_language(C)
add_executable(empty empty.c)
target_compile_options(empty PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall>)
