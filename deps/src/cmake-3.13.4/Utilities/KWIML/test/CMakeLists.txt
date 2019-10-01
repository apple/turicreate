#
# Copyright Kitware, Inc.
# Distributed under the OSI-approved BSD 3-Clause License.
# See accompanying file Copyright.txt for details.
#
if(NOT KWIML_TEST_PREFIX)
  set(KWIML_TEST_PREFIX kwiml)
endif()

# Suppress printf/scanf format warnings; we test if the sizes match.
foreach(lang C CXX)
  if(KWIML_LANGUAGE_${lang} AND CMAKE_${lang}_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} -Wno-format -Wno-format-security")
  endif()
endforeach()

if(KWIML_LANGUAGE_C)
  set(test_srcs test.c)
else()
  set(test_srcs test.cxx)
endif()
if(KWIML_LANGUAGE_C)
  list(APPEND test_defs KWIML_LANGUAGE_C)
  list(APPEND test_srcs
    test_abi_C.c
    test_int_C.c
    test_include_C.c
    )
endif()
if(KWIML_LANGUAGE_CXX)
  list(APPEND test_defs KWIML_LANGUAGE_CXX)
  list(APPEND test_srcs
    test_abi_CXX.cxx
    test_int_CXX.cxx
    test_include_CXX.cxx
    )
endif()

add_executable(kwiml_test ${test_srcs})
set_property(TARGET kwiml_test PROPERTY COMPILE_DEFINITIONS ${test_defs})
set_property(TARGET kwiml_test PROPERTY C_INCLUDE_WHAT_YOU_USE "")
set_property(TARGET kwiml_test PROPERTY CXX_INCLUDE_WHAT_YOU_USE "")
set_property(TARGET kwiml_test PROPERTY
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME ${KWIML_TEST_PREFIX}.test COMMAND kwiml_test)
set_property(TEST ${KWIML_TEST_PREFIX}.test PROPERTY LABELS ${KWIML_TEST_LABELS})

# Xcode 2.x forgets to create the output directory before linking
# the individual architectures.
if(CMAKE_OSX_ARCHITECTURES AND XCODE
    AND NOT "${XCODE_VERSION}" MATCHES "^[^12]")
  add_custom_command(
    TARGET kwiml_test
    PRE_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CFG_INTDIR}"
    )
endif()
