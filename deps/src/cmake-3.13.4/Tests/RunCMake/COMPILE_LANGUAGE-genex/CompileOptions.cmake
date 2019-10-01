
enable_language(CXX)

add_executable(main main.cpp)
target_compile_options(main PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-DANYTHING>)
