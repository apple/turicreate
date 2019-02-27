add_executable(BadInvalidName6 ../main.cpp)
include_directories("$<TARGET_PROPERTY:,ValidProperty>")
