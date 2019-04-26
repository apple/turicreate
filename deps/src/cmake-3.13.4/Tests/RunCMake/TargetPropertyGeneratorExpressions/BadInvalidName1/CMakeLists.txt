add_executable(BadInvalidName1 ../main.cpp)
include_directories("$<TARGET_PROPERTY:Invali/dTarget,INCLUDE_DIRECTORIES>")
