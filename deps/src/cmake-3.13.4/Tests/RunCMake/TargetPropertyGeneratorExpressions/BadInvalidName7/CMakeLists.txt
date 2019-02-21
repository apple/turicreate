add_executable(BadInvalidName7 ../main.cpp)
include_directories("$<TARGET_PROPERTY:BadInvalidName7,>")
