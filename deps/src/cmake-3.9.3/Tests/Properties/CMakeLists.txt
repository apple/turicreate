# a simple CXX only test case
cmake_minimum_required (VERSION 2.6)
project (Properties)

# these first three tests really test both properties and the management of
# cmSourceFile objects by CMake.

# test properties on a build tree file that is relative (yuck)
configure_file(properties.h.in "${Properties_BINARY_DIR}/properties.h")
set_source_files_properties(properties.h PROPERTIES TEST1 1)
get_source_file_property(RESULT1 properties.h TEST1)

# test properties on a headerfile in the source tree
# accessed without an extenion (also yuck)
set_source_files_properties(properties2 PROPERTIES TEST2 1)
get_source_file_property(RESULT2 properties2 TEST2)

# test properties on a relative source that is not generated
set_source_files_properties(SubDir/properties3.cxx PROPERTIES TEST3 1)
get_source_file_property(RESULT3 SubDir/properties3.cxx TEST3)

include_directories("${Properties_SOURCE_DIR}" "${Properties_BINARY_DIR}")


# test generic property interfaces
get_property(GLOBALRESULT GLOBAL PROPERTY GLOBALTEST DEFINED)
if (GLOBALRESULT)
    message(SEND_ERROR "Error: global prop defined when it should not be, "
            "result is GLOBALRESULT=${GLOBALRESULT}")
endif ()

define_property(GLOBAL PROPERTY GLOBALTEST
  BRIEF_DOCS "A test property"
  FULL_DOCS "A long description of this test property"
  )

get_property(GLOBALRESULT GLOBAL PROPERTY GLOBALTEST DEFINED)
if (NOT GLOBALRESULT)
    message(SEND_ERROR "Error: global prop not defined "
            "result is GLOBALRESULT=${GLOBALRESULT}")
endif ()

set_property(GLOBAL PROPERTY GLOBALTEST 1)
set_property(DIRECTORY PROPERTY DIRECTORYTEST 1)
set_property(SOURCE SubDir/properties3.cxx PROPERTY SOURCETEST 1)
get_property(GLOBALRESULT GLOBAL PROPERTY GLOBALTEST)
get_property(DIRECTORYRESULT DIRECTORY PROPERTY DIRECTORYTEST)
get_property(SOURCERESULT
  SOURCE SubDir/properties3.cxx
  PROPERTY SOURCETEST
  )

if (RESULT1 AND RESULT2 AND RESULT3 AND GLOBALRESULT AND
    DIRECTORYRESULT AND SOURCERESULT)
  add_executable (Properties SubDir/properties3.cxx properties)
else ()
  message(SEND_ERROR
    "Error: test results are RESULT1=${RESULT1} RESULT2=${RESULT2} "
    "RESULT3=${RESULT3} GLOBALRESULT=${GLOBALRESULT} "
    "DIRECTORYRESULT=${DIRECTORYRESULT} "
    "SOURCERESULT=${SOURCERESULT}")
endif ()

# test the target property
set_property(TARGET Properties PROPERTY TARGETTEST 1)
get_property(TARGETRESULT TARGET Properties PROPERTY TARGETTEST)
if (NOT TARGETRESULT)
    message(SEND_ERROR
      "Error: target result is TARGETRESULT=${TARGETRESULT}")
endif ()

# test APPEND and APPEND_STRING set_property()
set_property(TARGET Properties PROPERTY FOO foo)
set_property(TARGET Properties PROPERTY BAR bar)
set_property(TARGET Properties APPEND PROPERTY FOO 123)
set_property(TARGET Properties APPEND_STRING PROPERTY BAR 456)

get_property(APPEND_RESULT TARGET Properties PROPERTY FOO)
if (NOT "${APPEND_RESULT}" STREQUAL "foo;123")
    message(SEND_ERROR
      "Error: target result is APPEND_RESULT=${APPEND_RESULT}")
endif ()

get_property(APPEND_STRING_RESULT TARGET Properties PROPERTY BAR)
if (NOT "${APPEND_STRING_RESULT}" STREQUAL "bar456")
    message(SEND_ERROR
      "Error: target result is APPEND_STRING_RESULT=${APPEND_STRING_RESULT}")
endif ()

# test get_property SET
get_property(TARGETRESULT TARGET Properties PROPERTY TARGETTEST SET)
if (NOT TARGETRESULT)
    message(SEND_ERROR
      "Error: target prop not set, result is TARGETRESULT=${TARGETRESULT}")
endif ()

# test unsetting a property
set_property(TARGET Properties PROPERTY TARGETTEST)
get_property(TARGETRESULT TARGET Properties PROPERTY TARGETTEST SET)
if (TARGETRESULT)
    message(SEND_ERROR "Error: target prop not unset, "
            "result is TARGETRESULT=${TARGETRESULT}")
endif ()



# test the target SOURCES property
get_property(Properties_SOURCES TARGET Properties PROPERTY SOURCES)
set_source_files_properties(${Properties_SOURCES} PROPERTIES TEST4 1)
get_source_file_property(RESULT4 properties.h TEST4)
if(NOT RESULT4)
  message(SEND_ERROR "Error: target result is"
    " RESULT4=${RESULT4}"
    " Properties_SOURCES=[${Properties_SOURCES}]")
endif()

# test CACHE properties
macro(check_cache_props)
  foreach(prop VALUE TYPE HELPSTRING ADVANCED STRINGS)
    get_property(result CACHE SOME_ENTRY PROPERTY ${prop})
    if(NOT "x${result}" STREQUAL "x${expect_${prop}}")
      message(SEND_ERROR "CACHE property ${prop} is [${result}], not [${expect_${prop}}]")
    endif()
  endforeach()
endmacro()
set(expect_VALUE "ON")
set(expect_TYPE "BOOL")
set(expect_HELPSTRING "sample cache entry")
set(expect_ADVANCED 0)
set(expect_STRINGS "")
set(SOME_ENTRY "${expect_VALUE}" CACHE ${expect_TYPE} "${expect_HELPSTRING}" FORCE)
mark_as_advanced(CLEAR SOME_ENTRY)
set_property(CACHE SOME_ENTRY PROPERTY STRINGS "")
check_cache_props()
set(expect_VALUE "Some string")
set(expect_TYPE "STRING")
set(expect_HELPSTRING "sample cache entry help")
set(expect_ADVANCED 1)
set(expect_STRINGS "Some string;Some other string;Some third string")
set_property(CACHE SOME_ENTRY PROPERTY TYPE "${expect_TYPE}")
set_property(CACHE SOME_ENTRY PROPERTY HELPSTRING "${expect_HELPSTRING}")
set_property(CACHE SOME_ENTRY PROPERTY VALUE "${expect_VALUE}")
set_property(CACHE SOME_ENTRY PROPERTY ADVANCED "${expect_ADVANCED}")
set_property(CACHE SOME_ENTRY PROPERTY STRINGS "${expect_STRINGS}")
check_cache_props()

add_subdirectory(SubDir2)
