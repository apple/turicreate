file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/main.cpp"
    "int main() {return 0;}\n")
add_executable(test_prog "${CMAKE_CURRENT_BINARY_DIR}/main.cpp")

install(TARGETS test_prog DESTINATION foo COMPONENT applications)

set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
