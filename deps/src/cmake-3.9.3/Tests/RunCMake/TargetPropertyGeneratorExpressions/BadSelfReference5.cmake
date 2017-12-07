
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/main.cpp"
           "int main(int, char **) { return 0; }\n")

add_executable(TargetPropertyGeneratorExpressions
           "${CMAKE_CURRENT_BINARY_DIR}/main.cpp")
set_property(TARGET TargetPropertyGeneratorExpressions
PROPERTY
  COMPILE_DEFINITIONS "$<TARGET_PROPERTY:COMPILE_DEFINITIONS>"
)
