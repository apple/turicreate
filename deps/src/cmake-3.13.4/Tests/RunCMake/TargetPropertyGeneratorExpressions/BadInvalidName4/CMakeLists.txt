add_executable(BadInvalidName4 ../main.cpp)
include_directories("$<TARGET_PROPERTY:BadInvalidName4,Invali/dProperty>")
