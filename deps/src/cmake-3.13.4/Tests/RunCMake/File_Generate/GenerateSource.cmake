
enable_language(CXX)

# Ensure re-generation
file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/main.cpp")

file(GENERATE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/main.cpp"
  CONTENT "int main() { return 0; }\n"
)

add_executable(mn "${CMAKE_CURRENT_BINARY_DIR}/main.cpp")
