cmake_minimum_required(VERSION 3.1)
project(TestFindBoost CXX)
include(CTest)

find_package(Boost REQUIRED COMPONENTS filesystem thread)

add_executable(test_boost_tgt main.cxx)
target_link_libraries(test_boost_tgt
                      Boost::dynamic_linking
                      Boost::disable_autolinking
                      Boost::filesystem
                      Boost::thread)
add_test(NAME test_boost_tgt COMMAND test_boost_tgt)

add_executable(test_boost_var main.cxx)
target_include_directories(test_boost_var PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(test_boost_var PRIVATE ${Boost_FILESYSTEM_LIBRARIES} ${Boost_SYSTEM_LIBRARIES} ${Boost_THREAD_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME test_boost_var COMMAND test_boost_var)
