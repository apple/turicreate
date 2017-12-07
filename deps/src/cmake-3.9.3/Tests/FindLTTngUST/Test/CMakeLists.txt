cmake_minimum_required(VERSION 3.5)
project(TestFindLTTngUST C)
include(CTest)

find_package(LTTngUST REQUIRED)

add_definitions(-DCMAKE_EXPECTED_LTTNGUST_VERSION="${LTTNGUST_VERSION_STRING}")
add_definitions(-DCMAKE_LTTNGUST_HAS_TRACEF="${LTTNGUST_HAS_TRACEF}")
add_definitions(-DCMAKE_LTTNGUST_HAS_TRACELOG="${LTTNGUST_HAS_TRACELOG}")

add_executable(test_tgt main.c)
target_link_libraries(test_tgt LTTng::UST)
add_test(NAME test_tgt COMMAND test_tgt)

add_executable(test_var main.c)
target_include_directories(test_var PRIVATE ${LTTNGUST_INCLUDE_DIRS})
target_link_libraries(test_var PRIVATE ${LTTNGUST_LIBRARIES})
add_test(NAME test_var COMMAND test_var)
