add_executable(BadSelfReference3 ../main.cpp)
include_directories("$<TARGET_PROPERTY:BadSelfReference3,INCLUDE_DIRECTORIES>")
