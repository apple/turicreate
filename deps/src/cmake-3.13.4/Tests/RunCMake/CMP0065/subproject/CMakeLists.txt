cmake_minimum_required(VERSION 3.3)

project(TestPolicyCMP0065 C)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS BADFLAGS)

#----------------------------------------------------------------------
cmake_policy(SET CMP0065 OLD)
add_executable(FooOLDBad1 main.c)

#----------------------------------------------------------------------
cmake_policy(SET CMP0065 OLD)
add_executable(FooOLDBad2 main.c)
set_target_properties(FooOLDBad2 PROPERTIES ENABLE_EXPORTS ON)

#----------------------------------------------------------------------
cmake_policy(SET CMP0065 NEW)
add_executable(FooNEWGood main.c)

#----------------------------------------------------------------------
cmake_policy(SET CMP0065 NEW)
add_executable(FooNEWBad main.c)
set_target_properties(FooNEWBad PROPERTIES ENABLE_EXPORTS ON)
