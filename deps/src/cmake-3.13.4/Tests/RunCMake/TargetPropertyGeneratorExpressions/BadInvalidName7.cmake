
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/main.cpp"
           "int main(int, char **) { return 0; }\n")

add_executable(foo "${CMAKE_CURRENT_BINARY_DIR}/main.cpp")

add_executable(TargetPropertyGeneratorExpressions
           "${CMAKE_CURRENT_BINARY_DIR}/main.cpp")
include_directories("$<TARGET_PROPERTY:foo,>")
