cmake_minimum_required(VERSION 3.1)
project(TestFindBoostHeaders CXX)
include(CTest)

find_package(Boost REQUIRED)

add_executable(test_boost_headers_tgt main.cxx)
target_link_libraries(test_boost_headers_tgt
                      Boost::boost)
add_test(NAME test_boost_headers_tgt COMMAND test_boost_headers_tgt)
