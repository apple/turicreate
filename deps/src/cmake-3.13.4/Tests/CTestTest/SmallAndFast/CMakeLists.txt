cmake_minimum_required(VERSION 2.8)
project(SmallAndFast)

include(CTest)

add_executable(echoargs echoargs.c)
set_property(SOURCE echoargs.c APPEND PROPERTY LABELS SourceLabel Everything)
set_property(TARGET echoargs APPEND PROPERTY LABELS TargetLabel Everything)

add_test(test0 echoargs)
set_property(TEST test0 APPEND PROPERTY LABELS TestLabel 0ArgTest Everything)

add_test(test1 echoargs 1)
set_property(TEST test1 APPEND PROPERTY LABELS TestLabel 1ArgTest Everything)

add_test(test2 echoargs 1 2)
set_property(TEST test2 APPEND PROPERTY LABELS TestLabel 2ArgTest Everything)

if(SAF_INTENTIONAL_COMPILE_ERROR)
  add_executable(ice intentional_compile_error.cxx)
endif()

if(SAF_INTENTIONAL_COMPILE_WARNING)
  add_executable(icw intentional_compile_warning.cxx)
endif()
