set(CMAKE_BUILD_WITH_INSTALL_RPATH 1)

# PGI compiler doesn't add build id to binaries by default
if(CMAKE_CXX_COMPILER_ID STREQUAL "PGI")
  string(APPEND CMAKE_EXE_LINKER_FLAGS "-Wl,--build-id")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS "-Wl,--build-id")
endif()

if(NOT RunCMake_SUBTEST_SUFFIX STREQUAL "no_components")
  set(CPACK_RPM_COMPONENT_INSTALL "ON")
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

if(RunCMake_SUBTEST_SUFFIX STREQUAL "valid"
  OR RunCMake_SUBTEST_SUFFIX STREQUAL "no_main_component"
  OR RunCMake_SUBTEST_SUFFIX STREQUAL "no_debuginfo")
  install(FILES CMakeLists.txt DESTINATION bar COMPONENT headers)
  install(TARGETS test_lib DESTINATION bas COMPONENT libs)
elseif(RunCMake_SUBTEST_SUFFIX STREQUAL "one_component"
  OR RunCMake_SUBTEST_SUFFIX STREQUAL "one_component_no_debuginfo")
  set(CPACK_COMPONENTS_ALL applications)
endif()

set(CPACK_RPM_DEBUGINFO_SINGLE_PACKAGE ON)

if(RunCMake_SUBTEST_SUFFIX STREQUAL "valid"
  OR RunCMake_SUBTEST_SUFFIX STREQUAL "one_component_main"
  OR RunCMake_SUBTEST_SUFFIX STREQUAL "no_debuginfo")
  set(CPACK_RPM_MAIN_COMPONENT "applications")
  set(CPACK_RPM_APPLICATIONS_FILE_NAME "RPM-DEFAULT")
endif()

if(RunCMake_SUBTEST_SUFFIX STREQUAL "valid"
  OR RunCMake_SUBTEST_SUFFIX STREQUAL "no_main_component"
  OR RunCMake_SUBTEST_SUFFIX STREQUAL "one_component")
  set(CPACK_RPM_APPLICATIONS_DEBUGINFO_PACKAGE ON)
  set(CPACK_RPM_LIBS_DEBUGINFO_PACKAGE ON)
endif()

set(CPACK_RPM_BUILD_SOURCE_DIRS_PREFIX "/src")
