include(${CMAKE_CURRENT_LIST_DIR}/test_utils.cmake)

# example from the documentation
# OPTIONAL is a keyword and therefore terminates the definition of
# the multi-value DEFINITION before even a single value has been added

set(options OPTIONAL FAST)
set(oneValueArgs DESTINATION RENAME)
set(multiValueArgs TARGETS CONFIGURATIONS)
cmake_parse_arguments(MY_INSTALL "${options}" "${oneValueArgs}"
                      "${multiValueArgs}"
                      TARGETS foo DESTINATION OPTIONAL)

TEST(MY_INSTALL_DESTINATION UNDEFINED)
TEST(MY_INSTALL_OPTIONAL TRUE)

macro(foo)
  set(_options )
  set(_oneValueArgs FOO)
  set(_multiValueArgs )
  cmake_parse_arguments(_FOO2 "${_options}"
                              "${_oneValueArgs}"
                              "${_multiValueArgs}"
                              "${ARGN}")
  cmake_parse_arguments(_FOO1 "${_options}"
                              "${_oneValueArgs}"
                              "${_multiValueArgs}"
                              ${ARGN})
endmacro()

foo(FOO foo)
TEST(_FOO1_FOO foo)
TEST(_FOO2_FOO foo)

# Make sure a list is split
foo(FOO "foo;bar")
TEST(_FOO1_FOO foo)
TEST(_FOO1_UNPARSED_ARGUMENTS "bar")
TEST(_FOO2_FOO foo)
TEST(_FOO2_UNPARSED_ARGUMENTS "bar")

# Do not split if argn is quoted
foo(FOO "foo\\;bar")
TEST(_FOO1_FOO foo)
TEST(_FOO1_UNPARSED_ARGUMENTS "bar")
TEST(_FOO2_FOO foo;bar)
TEST(_FOO2_UNPARSED_ARGUMENTS "UNDEFINED")

# Do not split if argn is quoted
foo(FOO "foo\\\\;bar")
TEST(_FOO1_FOO foo)
TEST(_FOO1_UNPARSED_ARGUMENTS "bar")
TEST(_FOO2_FOO foo;bar)
TEST(_FOO2_UNPARSED_ARGUMENTS "UNDEFINED")
