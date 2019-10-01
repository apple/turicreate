
cmake_minimum_required(VERSION 2.8.4)
project(${RunCMake_TEST} CXX)

# MSVC creates extra targets which pollute the stderr unless we set this.
set(CMAKE_SUPPRESS_REGENERATION TRUE)

include(${RunCMake_TEST}.cmake)
