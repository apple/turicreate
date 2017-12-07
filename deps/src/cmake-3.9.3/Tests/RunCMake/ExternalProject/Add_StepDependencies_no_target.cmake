cmake_minimum_required(VERSION ${CMAKE_VERSION})

include(ExternalProject)

ExternalProject_Add(BAR URL https://cmake.org/bar.tar.gz)

ExternalProject_Add(FOO URL https://cmake.org/foo.tar.gz STEP_TARGETS update)
ExternalProject_Add_Step(FOO do_something COMMAND ${CMAKE_COMMAND} -E echo "Doing something")
ExternalProject_Add_Step(FOO do_something_else COMMAND ${CMAKE_COMMAND} -E echo "Doing something else")
ExternalProject_Add_StepTargets(FOO do_something)
