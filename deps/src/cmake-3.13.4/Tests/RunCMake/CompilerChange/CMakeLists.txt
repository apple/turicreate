cmake_minimum_required(VERSION 2.8.4)
if(NOT RunCMake_TEST)
  set(RunCMake_TEST "$ENV{RunCMake_TEST}") # needed when cache is deleted
endif()
project(${RunCMake_TEST} NONE)
include(${RunCMake_TEST}.cmake)
