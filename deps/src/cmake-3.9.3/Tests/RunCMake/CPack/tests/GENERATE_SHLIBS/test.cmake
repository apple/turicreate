set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")

set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS "ON")

set(CMAKE_BUILD_WITH_INSTALL_RPATH 1)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test_lib.hpp"
    "int test_lib();\n")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test_lib.cpp"
    "#include \"test_lib.hpp\"\nint test_lib() {return 0;}\n")
add_library(test_lib SHARED "${CMAKE_CURRENT_BINARY_DIR}/test_lib.cpp")
set_target_properties(test_lib PROPERTIES SOVERSION "0.8")

install(TARGETS test_lib DESTINATION foo COMPONENT libs)
