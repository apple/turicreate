# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
GoogleTest
----------

This module defines functions to help use the Google Test infrastructure.

.. command:: gtest_add_tests

  Automatically add tests with CTest by scanning source code for Google Test
  macros::

    gtest_add_tests(TARGET target
                    [SOURCES src1...]
                    [EXTRA_ARGS arg1...]
                    [WORKING_DIRECTORY dir]
                    [TEST_PREFIX prefix]
                    [TEST_SUFFIX suffix]
                    [SKIP_DEPENDENCY]
                    [TEST_LIST outVar]
    )

  ``TARGET target``
    This must be a known CMake target. CMake will substitute the location of
    the built executable when running the test.

  ``SOURCES src1...``
    When provided, only the listed files will be scanned for test cases. If
    this option is not given, the :prop_tgt:`SOURCES` property of the
    specified ``target`` will be used to obtain the list of sources.

  ``EXTRA_ARGS arg1...``
    Any extra arguments to pass on the command line to each test case.

  ``WORKING_DIRECTORY dir``
    Specifies the directory in which to run the discovered test cases. If this
    option is not provided, the current binary directory is used.

  ``TEST_PREFIX prefix``
    Allows the specified ``prefix`` to be prepended to the name of each
    discovered test case. This can be useful when the same source files are
    being used in multiple calls to ``gtest_add_test()`` but with different
    ``EXTRA_ARGS``.

  ``TEST_SUFFIX suffix``
    Similar to ``TEST_PREFIX`` except the ``suffix`` is appended to the name of
    every discovered test case. Both ``TEST_PREFIX`` and ``TEST_SUFFIX`` can be
    specified.

  ``SKIP_DEPENDENCY``
    Normally, the function creates a dependency which will cause CMake to be
    re-run if any of the sources being scanned are changed. This is to ensure
    that the list of discovered tests is updated. If this behavior is not
    desired (as may be the case while actually writing the test cases), this
    option can be used to prevent the dependency from being added.

  ``TEST_LIST outVar``
    The variable named by ``outVar`` will be populated in the calling scope
    with the list of discovered test cases. This allows the caller to do things
    like manipulate test properties of the discovered tests.

  .. code-block:: cmake

    include(GoogleTest)
    add_executable(FooTest FooUnitTest.cxx)
    gtest_add_tests(TARGET      FooTest
                    TEST_SUFFIX .noArgs
                    TEST_LIST   noArgsTests
    )
    gtest_add_tests(TARGET      FooTest
                    EXTRA_ARGS  --someArg someValue
                    TEST_SUFFIX .withArgs
                    TEST_LIST   withArgsTests
    )
    set_tests_properties(${noArgsTests}   PROPERTIES TIMEOUT 10)
    set_tests_properties(${withArgsTests} PROPERTIES TIMEOUT 20)

  For backward compatibility reasons, the following form is also supported::

    gtest_add_tests(exe args files...)

  ``exe``
    The path to the test executable or the name of a CMake target.
  ``args``
    A ;-list of extra arguments to be passed to executable.  The entire
    list must be passed as a single argument.  Enclose it in quotes,
    or pass ``""`` for no arguments.
  ``files...``
    A list of source files to search for tests and test fixtures.
    Alternatively, use ``AUTO`` to specify that ``exe`` is the name
    of a CMake executable target whose sources should be scanned.

  .. code-block:: cmake

    include(GoogleTest)
    set(FooTestArgs --foo 1 --bar 2)
    add_executable(FooTest FooUnitTest.cxx)
    gtest_add_tests(FooTest "${FooTestArgs}" AUTO)

#]=======================================================================]

function(gtest_add_tests)

  if (ARGC LESS 1)
    message(FATAL_ERROR "No arguments supplied to gtest_add_tests()")
  endif()

  set(options
      SKIP_DEPENDENCY
  )
  set(oneValueArgs
      TARGET
      WORKING_DIRECTORY
      TEST_PREFIX
      TEST_SUFFIX
      TEST_LIST
  )
  set(multiValueArgs
      SOURCES
      EXTRA_ARGS
  )
  set(allKeywords ${options} ${oneValueArgs} ${multiValueArgs})

  unset(sources)
  if("${ARGV0}" IN_LIST allKeywords)
    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    set(autoAddSources YES)
  else()
    # Non-keyword syntax, convert to keyword form
    if (ARGC LESS 3)
      message(FATAL_ERROR "gtest_add_tests() without keyword options requires at least 3 arguments")
    endif()
    set(ARGS_TARGET     "${ARGV0}")
    set(ARGS_EXTRA_ARGS "${ARGV1}")
    if(NOT "${ARGV2}" STREQUAL "AUTO")
      set(ARGS_SOURCES "${ARGV}")
      list(REMOVE_AT ARGS_SOURCES 0 1)
    endif()
  endif()

  # The non-keyword syntax allows the first argument to be an arbitrary
  # executable rather than a target if source files are also provided. In all
  # other cases, both forms require a target.
  if(NOT TARGET "${ARGS_TARGET}" AND NOT ARGS_SOURCES)
    message(FATAL_ERROR "${ARGS_TARGET} does not define an existing CMake target")
  endif()
  if(NOT ARGS_WORKING_DIRECTORY)
    unset(workDir)
  else()
    set(workDir WORKING_DIRECTORY "${ARGS_WORKING_DIRECTORY}")
  endif()

  if(NOT ARGS_SOURCES)
    get_property(ARGS_SOURCES TARGET ${ARGS_TARGET} PROPERTY SOURCES)
  endif()

  unset(testList)

  set(gtest_case_name_regex ".*\\( *([A-Za-z_0-9]+) *, *([A-Za-z_0-9]+) *\\).*")
  set(gtest_test_type_regex "(TYPED_TEST|TEST_?[FP]?)")

  foreach(source IN LISTS ARGS_SOURCES)
    if(NOT ARGS_SKIP_DEPENDENCY)
      set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${source})
    endif()
    file(READ "${source}" contents)
    string(REGEX MATCHALL "${gtest_test_type_regex} *\\(([A-Za-z_0-9 ,]+)\\)" found_tests ${contents})
    foreach(hit ${found_tests})
      string(REGEX MATCH "${gtest_test_type_regex}" test_type ${hit})

      # Parameterized tests have a different signature for the filter
      if("x${test_type}" STREQUAL "xTEST_P")
        string(REGEX REPLACE ${gtest_case_name_regex}  "*/\\1.\\2/*" gtest_test_name ${hit})
      elseif("x${test_type}" STREQUAL "xTEST_F" OR "x${test_type}" STREQUAL "xTEST")
        string(REGEX REPLACE ${gtest_case_name_regex} "\\1.\\2" gtest_test_name ${hit})
      elseif("x${test_type}" STREQUAL "xTYPED_TEST")
        string(REGEX REPLACE ${gtest_case_name_regex} "\\1/*.\\2" gtest_test_name ${hit})
      else()
        message(WARNING "Could not parse GTest ${hit} for adding to CTest.")
        continue()
      endif()

      # Make sure tests disabled in GTest get disabled in CTest
      if(gtest_test_name MATCHES "(^|\\.)DISABLED_")
        # Add the disabled test if CMake is new enough
        # Note that this check is to allow backwards compatibility so this
        # module can be copied locally in projects to use with older CMake
        # versions
        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.8.20170401)
          string(REGEX REPLACE
                 "(^|\\.)DISABLED_" "\\1"
                 orig_test_name "${gtest_test_name}"
          )
          set(ctest_test_name
              ${ARGS_TEST_PREFIX}${orig_test_name}${ARGS_TEST_SUFFIX}
          )
          add_test(NAME ${ctest_test_name}
                   ${workDir}
                   COMMAND ${ARGS_TARGET}
                     --gtest_also_run_disabled_tests
                     --gtest_filter=${gtest_test_name}
                     ${ARGS_EXTRA_ARGS}
          )
          set_tests_properties(${ctest_test_name} PROPERTIES DISABLED TRUE)
          list(APPEND testList ${ctest_test_name})
        endif()
      else()
        set(ctest_test_name ${ARGS_TEST_PREFIX}${gtest_test_name}${ARGS_TEST_SUFFIX})
        add_test(NAME ${ctest_test_name}
                 ${workDir}
                 COMMAND ${ARGS_TARGET}
                   --gtest_filter=${gtest_test_name}
                   ${ARGS_EXTRA_ARGS}
        )
        list(APPEND testList ${ctest_test_name})
      endif()
    endforeach()
  endforeach()

  if(ARGS_TEST_LIST)
    set(${ARGS_TEST_LIST} ${testList} PARENT_SCOPE)
  endif()

endfunction()
