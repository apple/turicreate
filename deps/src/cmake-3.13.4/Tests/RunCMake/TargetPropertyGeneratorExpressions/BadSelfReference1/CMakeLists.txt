add_executable(BadSelfReference1 ../main.cpp)
include_directories("$<TARGET_PROPERTY:INCLUDE_DIRECTORIES>")
