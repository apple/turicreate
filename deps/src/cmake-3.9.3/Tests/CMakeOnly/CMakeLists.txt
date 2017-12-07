configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Test.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/Test.cmake @ONLY)

macro(add_CMakeOnly_test test)
  add_test(CMakeOnly.${test} ${CMAKE_CMAKE_COMMAND}
    -DTEST=${test}
    -P ${CMAKE_CURRENT_BINARY_DIR}/Test.cmake
    )
endmacro()

add_CMakeOnly_test(LinkInterfaceLoop)
# If a bug is introduced in CMake that causes an infinite loop while
# analyzing LinkInterfaceLoop then don't let the test run too long.
# Use an option to customize it so that the timeout can be extended
# on busy machines.
if(NOT DEFINED CMake_TEST_CMakeOnly.LinkInterfaceLoop_TIMEOUT)
  set(CMake_TEST_CMakeOnly.LinkInterfaceLoop_TIMEOUT 90)
endif()
set_property(TEST CMakeOnly.LinkInterfaceLoop PROPERTY TIMEOUT ${CMake_TEST_CMakeOnly.LinkInterfaceLoop_TIMEOUT})

add_CMakeOnly_test(CheckSymbolExists)

add_CMakeOnly_test(CheckCXXSymbolExists)

add_CMakeOnly_test(CheckCXXCompilerFlag)

add_CMakeOnly_test(CheckLanguage)

add_CMakeOnly_test(CheckStructHasMember)

add_CMakeOnly_test(CompilerIdC)
add_CMakeOnly_test(CompilerIdCXX)
if(CMAKE_Fortran_COMPILER)
  add_CMakeOnly_test(CompilerIdFortran)
endif()
if(CMAKE_GENERATOR MATCHES "Visual Studio ([^89]|[89][0-9])")
  add_CMakeOnly_test(CompilerIdCSharp)
endif()

add_test(CMakeOnly.AllFindModules ${CMAKE_CMAKE_COMMAND}
  -DTEST=AllFindModules
  -DCMAKE_ARGS=-DCMake_TEST_CMakeOnly.AllFindModules_NO_VERSION=${CMake_TEST_CMakeOnly.AllFindModules_NO_VERSION}
  -P ${CMAKE_CURRENT_BINARY_DIR}/Test.cmake
  )

add_CMakeOnly_test(SelectLibraryConfigurations)

add_CMakeOnly_test(TargetScope)

add_CMakeOnly_test(find_library)
add_CMakeOnly_test(find_path)

add_test(CMakeOnly.ProjectInclude ${CMAKE_CMAKE_COMMAND}
  -DTEST=ProjectInclude
  -DCMAKE_ARGS=-DCMAKE_PROJECT_ProjectInclude_INCLUDE=${CMAKE_CURRENT_SOURCE_DIR}/ProjectInclude/include.cmake
  -P ${CMAKE_CURRENT_BINARY_DIR}/Test.cmake
  )

include(${CMAKE_SOURCE_DIR}/Modules/CMakeParseArguments.cmake)

function(add_major_test module)
  cmake_parse_arguments(MAJOR_TEST "NOLANG" "VERSION_VAR" "VERSIONS" ${ARGN})
  foreach (_version IN LISTS MAJOR_TEST_VERSIONS)
    add_test(CMakeOnly.MajorVersionSelection-${module}_${_version}
      ${CMAKE_CMAKE_COMMAND}
        -DTEST=MajorVersionSelection-${module}_${_version}
        -DTEST_SOURCE=MajorVersionSelection
        "-DCMAKE_ARGS=-DMAJOR_TEST_MODULE=${module};-DMAJOR_TEST_VERSION=${_version};-DMAJOR_TEST_NO_LANGUAGES=${MAJOR_TEST_NOLANG};-DMAJOR_TEST_VERSION_VAR=${MAJOR_TEST_VERSION_VAR}"
        -P ${CMAKE_CURRENT_BINARY_DIR}/Test.cmake
      )
  endforeach ()
endfunction()

add_major_test(PythonLibs VERSIONS 2 3 VERSION_VAR PYTHONLIBS_VERSION_STRING)
add_major_test(PythonInterp NOLANG VERSIONS 2 3 VERSION_VAR PYTHON_VERSION_STRING)
add_major_test(Qt VERSIONS 3 4 VERSION_VAR QT_VERSION_STRING)
