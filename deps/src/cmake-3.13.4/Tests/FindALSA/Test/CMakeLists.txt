cmake_minimum_required(VERSION 3.10)
project(TestFindALSA C)
include(CTest)

find_package(ALSA REQUIRED)

add_definitions(-DCMAKE_EXPECTED_ALSA_VERSION="${ALSA_VERSION_STRING}")

add_executable(test_tgt main.c)
target_link_libraries(test_tgt ALSA::ALSA)
add_test(NAME test_tgt COMMAND test_tgt)

add_executable(test_var main.c)
target_include_directories(test_var PRIVATE ${ALSA_INCLUDE_DIRS})
target_link_libraries(test_var PRIVATE ${ALSA_LIBRARIES})
add_test(NAME test_var COMMAND test_var)
