cmake_minimum_required(VERSION 3.7)

set(CMAKE_XCODE_GENERATE_SCHEME ON)

project(XcodeSchemaProperty CXX)

function(create_scheme_for_variable variable)
  set(CMAKE_XCODE_SCHEME_${variable} ON)
  add_executable(${variable} main.cpp)
endfunction()

create_scheme_for_variable(ADDRESS_SANITIZER)
create_scheme_for_variable(ADDRESS_SANITIZER_USE_AFTER_RETURN)
create_scheme_for_variable(THREAD_SANITIZER)
create_scheme_for_variable(THREAD_SANITIZER_STOP)
create_scheme_for_variable(UNDEFINED_BEHAVIOUR_SANITIZER)
create_scheme_for_variable(UNDEFINED_BEHAVIOUR_SANITIZER_STOP)
create_scheme_for_variable(DISABLE_MAIN_THREAD_CHECKER)
create_scheme_for_variable(MAIN_THREAD_CHECKER_STOP)

create_scheme_for_variable(MALLOC_SCRIBBLE)
create_scheme_for_variable(MALLOC_GUARD_EDGES)
create_scheme_for_variable(GUARD_MALLOC)
create_scheme_for_variable(ZOMBIE_OBJECTS)
create_scheme_for_variable(MALLOC_STACK)
create_scheme_for_variable(DYNAMIC_LINKER_API_USAGE)
create_scheme_for_variable(DYNAMIC_LIBRARY_LOADS)

function(create_scheme_for_property property value)
  set(XCODE_SCHEME_${property} ON)
  add_executable(${property} main.cpp)
  set_target_properties(${property} PROPERTIES XCODE_SCHEME_${property} "${value}")
endfunction()

create_scheme_for_property(EXECUTABLE myExecutable)
create_scheme_for_property(ARGUMENTS "--foo;--bar=baz")
create_scheme_for_property(ENVIRONMENT "FOO=foo;BAR=bar")
