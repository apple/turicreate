cmake_minimum_required(VERSION 3.10)
project(TestFindDoxygen VERSION 1.0 LANGUAGES NONE)
enable_testing()

find_package(Doxygen REQUIRED)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/main.cpp [[
/**
 * \file
 * \brief One C++ file w/ sample Doxygen comment just to produce any docs...
 */
]])

set(DOXYGEN_OUTPUT_DIRECTORY outDirWithout)
file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/${DOXYGEN_OUTPUT_DIRECTORY})
doxygen_add_docs(docsNoAll ${CMAKE_CURRENT_BINARY_DIR}/main.cpp)

set(DOXYGEN_OUTPUT_DIRECTORY outDirWith)
file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/${DOXYGEN_OUTPUT_DIRECTORY})
doxygen_add_docs(docsWithAll ALL ${CMAKE_CURRENT_BINARY_DIR}/main.cpp)

# Define tests cases that check whether targets were built
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dirExists.cmake [[
cmake_minimum_required(VERSION 3.11)
if(NOT EXISTS ${dirName})
    message(FATAL_ERROR "Directory does not exist: ${dirName}")
endif()
]])

add_test(NAME checkWith COMMAND
    ${CMAKE_COMMAND}
    -D dirName=${CMAKE_CURRENT_BINARY_DIR}/outDirWith
    -P dirExists.cmake
)
add_test(NAME checkWithout COMMAND
    ${CMAKE_COMMAND}
    -D dirName=${CMAKE_CURRENT_BINARY_DIR}/outDirWithout
    -P dirExists.cmake
)
set_tests_properties(checkWithout PROPERTIES
    WILL_FAIL TRUE
)
