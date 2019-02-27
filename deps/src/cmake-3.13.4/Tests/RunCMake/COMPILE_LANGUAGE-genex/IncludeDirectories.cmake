
enable_language(CXX)

add_executable(main main.cpp)
target_include_directories(main PRIVATE $<$<COMPILE_LANGUAGE:CXX>:anydir>)
