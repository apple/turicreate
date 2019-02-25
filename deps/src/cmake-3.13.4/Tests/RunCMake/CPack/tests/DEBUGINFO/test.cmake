set(CMAKE_BUILD_WITH_INSTALL_RPATH 1)

# PGI compiler doesn't add build id to binaries by default
if(CMAKE_CXX_COMPILER_ID STREQUAL "PGI")
  string(APPEND CMAKE_EXE_LINKER_FLAGS "-Wl,--build-id")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS "-Wl,--build-id")
endif()

set(CMAKE_BUILD_TYPE Debug)

# for rpm packages execute flag must be set for shared libs if debuginfo
# packages are generated
set(CPACK_RPM_INSTALL_WITH_EXEC TRUE)

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
set(CPACK_DEBIAN_APPLICATIONS_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_APPLICATIONS_DEBUGINFO_PACKAGE ON)

# test that components with debuginfo enabled still honor
# CPACK_PACKAGE_FILE_NAME setting
set(CPACK_RPM_PACKAGE_NAME "Debuginfo")
set(CPACK_PACKAGE_FILE_NAME "TestDinfo-pkg")
set(CPACK_RPM_LIBS_DEBUGINFO_PACKAGE ON)
set(CPACK_DEBIAN_PACKAGE_NAME "Debuginfo")
set(CPACK_DEBIAN_LIBS_DEBUGINFO_PACKAGE ON)

# test debuginfo package rename
set(CPACK_RPM_DEBUGINFO_FILE_NAME
  "@cpack_component@-DebugInfoPackage.rpm")
set(CPACK_RPM_APPLICATIONS_DEBUGINFO_FILE_NAME "RPM-DEFAULT")

set(CPACK_RPM_BUILD_SOURCE_DIRS_PREFIX "/src")
