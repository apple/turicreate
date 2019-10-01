project(PolicyScope C)
# No cmake_minimum_required(VERSION), it's in FindFoo.

#-----------------------------------------------------------------------------
# Helper function to report results.
function(check msg lhs rhs)
  if(NOT "${lhs}" STREQUAL "${rhs}")
    message(FATAL_ERROR "${msg}: expected [${lhs}], got [${rhs}]")
  endif()
endfunction()

#-----------------------------------------------------------------------------
# Test using a development framework that sets policies for us.

# Policy CMP0011 should not be set at this point.
cmake_policy(GET CMP0011 cmp)
check(CMP0011 "" "${cmp}")

# Put the test modules in the search path.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

# The included file should set policies for us.
find_package(Foo)

# Check policies set by the package.
cmake_policy(GET CMP0003 cmp)
check(CMP0003 "OLD" "${cmp}")
cmake_policy(GET CMP0002 cmp)
check(CMP0002 "NEW" "${cmp}")
cmake_policy(GET CMP0011 cmp)
check(CMP0011 "NEW" "${cmp}")

# Make sure an included file cannot change policies.
include(Bar)
cmake_policy(GET CMP0003 cmp)
check(CMP0003 "OLD" "${cmp}")

# Allow the included file to change policies.
include(Bar NO_POLICY_SCOPE)
cmake_policy(GET CMP0003 cmp)
check(CMP0003 "NEW" "${cmp}")

#-----------------------------------------------------------------------------
# Test function and macro policy recording.

# Create the functions in an isolated scope in which we change policies.
cmake_policy(PUSH)
if(1)
  # Change CMP0002
  cmake_policy(SET CMP0002 OLD)
  function(func1)
    # CMP0002 should be changed when this function is invoked
    cmake_policy(GET CMP0002 cmp)
    check(CMP0002 "OLD" "${cmp}")

    # The undocumented PARENT_SCOPE option sees the caller's setting.
    cmake_policy(GET CMP0002 cmp PARENT_SCOPE)
    check(CMP0002 "NEW" "${cmp}")
  endfunction()

  # Unset CMP0002
  cmake_policy(VERSION 2.4)
  macro(macro1)
    # CMP0002 should be unset when this macro is invoked
    cmake_policy(GET CMP0002 cmp)
    check(CMP0002 "" "${cmp}")

    # The undocumented PARENT_SCOPE option sees the caller's setting.
    cmake_policy(GET CMP0002 cmp PARENT_SCOPE)
    check(CMP0002 "NEW" "${cmp}")

    # Setting the policy should work here and also in the caller.
    cmake_policy(SET CMP0002 OLD)
    cmake_policy(GET CMP0002 cmp)
    check(CMP0002 "OLD" "${cmp}")
  endmacro()
endif()
cmake_policy(POP)

# CMP0002 should still be NEW in this context.
cmake_policy(GET CMP0002 cmp)
check(CMP0002 "NEW" "${cmp}")

# Check the recorded policies
func1()
macro1()

# The macro should have changed CMP0002.
cmake_policy(GET CMP0002 cmp)
check(CMP0002 "OLD" "${cmp}")

#-----------------------------------------------------------------------------
# Test CMAKE_POLICY_DEFAULT_CMP<NNNN> variable.
cmake_policy(PUSH)
  set(CMAKE_POLICY_DEFAULT_CMP0010 OLD) # ignored
  set(CMAKE_POLICY_DEFAULT_CMP0012 OLD) # honored
  set(CMAKE_POLICY_DEFAULT_CMP0013 NEW) # honored
  set(CMAKE_POLICY_DEFAULT_CMP0014 "")  # noop
  cmake_policy(VERSION 2.6.3)
  cmake_policy(GET CMP0010 cmp)
  check(CMP0010 "NEW" "${cmp}")
  cmake_policy(GET CMP0012 cmp)
  check(CMP0012 "OLD" "${cmp}")
  cmake_policy(GET CMP0013 cmp)
  check(CMP0013 "NEW" "${cmp}")
  cmake_policy(GET CMP0014 cmp)
  check(CMP0014 "" "${cmp}")
cmake_policy(POP)

#-----------------------------------------------------------------------------
# Dummy executable so the project can build and run.
add_executable(PolicyScope main.c)
