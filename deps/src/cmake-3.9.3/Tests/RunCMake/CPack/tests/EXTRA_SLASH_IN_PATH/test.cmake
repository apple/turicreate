set(CMAKE_BUILD_WITH_INSTALL_RPATH 1)

# PGI compiler doesn't add build id to binaries by default
if(CMAKE_CXX_COMPILER_ID STREQUAL "PGI")
  string(APPEND CMAKE_EXE_LINKER_FLAGS "-Wl,--build-id")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS "-Wl,--build-id")
endif()

set(CMAKE_BUILD_TYPE Debug)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test_lib.hpp"
    "int test_lib();\n")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/test_lib.cpp"
    "#include \"test_lib.hpp\"\nint test_lib() {return 0;}\n")
add_library(test_lib SHARED "${CMAKE_CURRENT_BINARY_DIR}/test_lib.cpp")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/main.cpp"
    "#include \"test_lib.hpp\"\nint main() {return test_lib();}\n")
add_executable(test_prog "${CMAKE_CURRENT_BINARY_DIR}/main.cpp")
target_link_libraries(test_prog test_lib)

install(TARGETS test_prog DESTINATION foo COMPONENT applications)
install(FILES CMakeLists.txt DESTINATION bar COMPONENT headers)
install(TARGETS test_lib DESTINATION bas COMPONENT libs)

set(CPACK_RPM_APPLICATIONS_FILE_NAME "RPM-DEFAULT")
set(CPACK_RPM_APPLICATIONS_DEBUGINFO_PACKAGE ON)
set(CPACK_RPM_LIBS_DEBUGINFO_PACKAGE ON)

# extra trailing slash at the end that should be removed
set(CPACK_RPM_BUILD_SOURCE_DIRS_PREFIX "/src/")

# combination should not cause //foo to apper as an relocation path
# should be only /foo (extra slashes cause path comparisons to fail)
set(CPACK_PACKAGING_INSTALL_PREFIX "/")
# extra trailing slash at the end that should be removed
set(CPACK_RPM_RELOCATION_PATHS "foo/")
