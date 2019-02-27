cmake_minimum_required(VERSION 2.8)
project(MacRuntimePath_A)

# a shared library
add_library(shared SHARED shared.cpp shared.h)
set_target_properties(shared PROPERTIES MACOSX_RPATH 1)

# a shared library with custom set @rpath
add_library(shared2 SHARED shared.cpp shared.h)
set_target_properties(shared2 PROPERTIES
  BUILD_WITH_INSTALL_RPATH 1 INSTALL_NAME_DIR "@rpath")

cmake_policy(SET CMP0042 NEW)

# a framework library
add_library(framework SHARED framework.cpp framework.h)
set_target_properties(framework PROPERTIES FRAMEWORK 1)

# another framework
add_library(framework2 SHARED framework2.cpp framework2.h)
set_target_properties(framework2 PROPERTIES FRAMEWORK 1)

# executable to test a shared library dependency with install rpaths
add_executable(test1 test1.cpp)
target_link_libraries(test1 shared)
set_target_properties(test1 PROPERTIES
  BUILD_WITH_INSTALL_RPATH 1 INSTALL_RPATH "@loader_path/../lib")

# executable to test a framework library dependency with install rpaths
add_executable(test2 test2.cpp)
target_link_libraries(test2 framework)
set_target_properties(test2 PROPERTIES
  BUILD_WITH_INSTALL_RPATH 1 INSTALL_RPATH "@loader_path/../lib")

# executable to test a framework library dependency with build tree rpaths
add_executable(test3 test3.cpp)
target_link_libraries(test3 framework)

# executable to test a framework library dependency with build tree rpaths
add_executable(test4 test1.cpp)
target_link_libraries(test4 shared2)

# executable to test a shared library dependency with build rpath
add_executable(test5 test1.cpp)

# avoid linking by 'target_link_libraries' so CMake
# will not be able to set correct RPATH automatically
add_dependencies(test5 shared)
target_link_libraries(test5 "$<TARGET_FILE:shared>")
set_target_properties(test5 PROPERTIES BUILD_RPATH "@loader_path/../lib")

set_target_properties(shared shared2 framework PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set_target_properties(test1 test2 test3 test4 test5 PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
foreach(config ${CMAKE_CONFIGURATION_TYPES})
  string(TOUPPER ${config} CONFIG)
  set_target_properties(shared shared2 framework PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY_${CONFIG}
      "${CMAKE_CURRENT_BINARY_DIR}/${config}/lib")
  set_target_properties(test1 test2 test3 test4 test5 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_${CONFIG}
      "${CMAKE_CURRENT_BINARY_DIR}/${config}/bin")
endforeach()

foreach(test test1 test2 test3 test4 test5)
  add_custom_target(${test}_run  ALL
    COMMAND ${test}
    DEPENDS ${test}
    )
endforeach()

export(TARGETS shared shared2 framework FILE "${CMAKE_CURRENT_BINARY_DIR}/exp.cmake")

install(TARGETS shared EXPORT MyExport DESTINATION lib)
install(TARGETS shared2 EXPORT MyExport DESTINATION lib2)
install(TARGETS framework EXPORT MyExport DESTINATION lib-fw)
install(TARGETS framework2 EXPORT MyExport DESTINATION lib-fw2)
install(EXPORT MyExport DESTINATION lib FILE exp.cmake)
