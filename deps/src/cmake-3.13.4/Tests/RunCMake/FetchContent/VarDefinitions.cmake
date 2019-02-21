unset(FETCHCONTENT_FULLY_DISCONNECTED CACHE)
unset(FETCHCONTENT_UPDATES_DISCONNECTED CACHE)
unset(FETCHCONTENT_QUIET CACHE)
unset(FETCHCONTENT_BASE_DIR CACHE)

include(FetchContent)

# Each of the cache entries should be defined and have the
# expected value. Be careful to check unset separately from a
# false value, since unset also equates to false.
if(FETCHCONTENT_FULLY_DISCONNECTED STREQUAL "")
  message(FATAL_ERROR "FETCHCONTENT_FULLY_DISCONNECTED not defined")
elseif(FETCHCONTENT_FULLY_DISCONNECTED)
  message(FATAL_ERROR "FETCHCONTENT_FULLY_DISCONNECTED not defaulted to OFF")
endif()

if(FETCHCONTENT_UPDATES_DISCONNECTED STREQUAL "")
  message(FATAL_ERROR "FETCHCONTENT_UPDATES_DISCONNECTED not defined")
elseif(FETCHCONTENT_UPDATES_DISCONNECTED)
  message(FATAL_ERROR "FETCHCONTENT_UPDATES_DISCONNECTED not defaulted to OFF")
endif()

if(FETCHCONTENT_QUIET STREQUAL "")
  message(FATAL_ERROR "FETCHCONTENT_QUIET not defined")
elseif(NOT FETCHCONTENT_QUIET)
  message(FATAL_ERROR "FETCHCONTENT_QUIET not defaulted to ON")
endif()

if(NOT FETCHCONTENT_BASE_DIR STREQUAL "${CMAKE_BINARY_DIR}/_deps")
  message(FATAL_ERROR "FETCHCONTENT_BASE_DIR has default value: "
          "${FETCHCONTENT_BASE_DIR}\n  Expected: ${CMAKE_BINARY_DIR}/_deps")
endif()

file(REMOVE_RECURSE ${FETCHCONTENT_BASE_DIR}/t1-subbuild)

# Use uppercase T1 test name to confirm conversion to lowercase
# for the t1_... variable names that get set
FetchContent_Declare(
  T1
  DOWNLOAD_COMMAND ${CMAKE_COMMAND} -E echo "Download command executed"
)
FetchContent_Populate(T1)

# Be careful to check both regular and cache variables. Since they have
# the same name, we can only confirm them separately by using get_property().
get_property(srcRegVarSet VARIABLE PROPERTY t1_SOURCE_DIR SET)
get_property(bldRegVarSet VARIABLE PROPERTY t1_BINARY_DIR SET)

get_property(srcCacheVarSet CACHE t1_SOURCE_DIR PROPERTY VALUE SET)
get_property(bldCacheVarSet CACHE t1_BINARY_DIR PROPERTY VALUE SET)

if(NOT srcRegVarSet)
  message(FATAL_ERROR "t1_SOURCE_DIR regular variable not set")
endif()
if(NOT bldRegVarSet)
  message(FATAL_ERROR "t1_BINARY_DIR regular variable not set")
endif()
if(srcCacheVarSet)
  message(FATAL_ERROR "t1_SOURCE_DIR cache variable unexpectedly set")
endif()
if(bldCacheVarSet)
  message(FATAL_ERROR "t1_BINARY_DIR cache variable unexpectedly set")
endif()

set(srcRegVar ${t1_SOURCE_DIR})
set(bldRegVar ${t1_BINARY_DIR})

if(NOT srcRegVar STREQUAL "${CMAKE_BINARY_DIR}/_deps/t1-src")
  message(FATAL_ERROR "Unexpected t1_SOURCE_DIR value: ${srcRegVar}\n"
                      "  Expected: ${CMAKE_BINARY_DIR}/_deps/t1-src")
endif()
if(NOT bldRegVar STREQUAL "${CMAKE_BINARY_DIR}/_deps/t1-build")
  message(FATAL_ERROR "Unexpected t1_BINARY_DIR value: ${bldRegVar}\n"
                      "  Expected: ${CMAKE_BINARY_DIR}/_deps/t1-build")
endif()
