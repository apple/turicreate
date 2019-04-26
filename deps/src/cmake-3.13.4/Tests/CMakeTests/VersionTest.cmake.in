set(min_ver 2.7.20090305)
cmake_minimum_required(VERSION ${min_ver})

if("${CMAKE_VERSION}" VERSION_LESS "${min_ver}")
  message(FATAL_ERROR
    "CMAKE_VERSION=[${CMAKE_VERSION}] is less than [${min_ver}]")
else()
  message("CMAKE_VERSION=[${CMAKE_VERSION}] is not less than [${min_ver}]")
endif()

set(EQUALV "1 1")
list(APPEND EQUALV "1.0 1")
list(APPEND EQUALV "1.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0 1")
list(APPEND EQUALV "1.2.3.4.5.6.7 1.2.3.4.5.6.7")
list(APPEND EQUALV "1.2.3.4.5.6.7.8.9 1.2.3.4.5.6.7.8.9")

foreach(v IN LISTS EQUALV)
  string(REGEX MATCH "(.*) (.*)" _dummy "${v}")
  # modify any of the operands to see the negative check also works
  if("${CMAKE_MATCH_1}.2" VERSION_EQUAL CMAKE_MATCH_2)
    message(FATAL_ERROR "${CMAKE_MATCH_1}.2 is equal ${CMAKE_MATCH_2}?")
  else()
    message(STATUS "${CMAKE_MATCH_1}.2 is not equal ${CMAKE_MATCH_2}")
  endif()

  if(CMAKE_MATCH_1 VERSION_EQUAL "${CMAKE_MATCH_2}.2")
    message(FATAL_ERROR "${CMAKE_MATCH_1} is equal ${CMAKE_MATCH_2}.2?")
  else()
    message(STATUS "${CMAKE_MATCH_1} is not equal ${CMAKE_MATCH_2}.2")
  endif()
endforeach()

# test the negative outcomes first, due to the implementation the positive
# allow some additional strings to pass that would not fail for the negative
# tests

list(APPEND EQUALV "1a 1")
list(APPEND EQUALV "1.1a 1.1")
list(APPEND EQUALV "1.0a 1")
list(APPEND EQUALV "1a 1.0")

foreach(v IN LISTS EQUALV)
  # check equal versions
  string(REGEX MATCH "(.*) (.*)" _dummy "${v}")
  if(CMAKE_MATCH_1 VERSION_EQUAL CMAKE_MATCH_2)
    message(STATUS "${CMAKE_MATCH_1} is equal ${CMAKE_MATCH_2}")
  else()
    message(FATAL_ERROR "${CMAKE_MATCH_1} is not equal ${CMAKE_MATCH_2}?")
  endif()

  # still equal, but inverted order of operands
  string(REGEX MATCH "(.*) (.*)" _dummy "${v}")
  if(CMAKE_MATCH_2 VERSION_EQUAL CMAKE_MATCH_1)
    message(STATUS "${CMAKE_MATCH_2} is equal ${CMAKE_MATCH_1}")
  else()
    message(FATAL_ERROR "${CMAKE_MATCH_2} is not equal ${CMAKE_MATCH_1}?")
  endif()
endforeach()

set(LESSV "1.2.3.4.5.6.7.8 1.2.3.4.5.6.7.9")
list(APPEND LESSV "1.2.3.4.5.6.7 1.2.3.4.5.6.7.9")
list(APPEND LESSV "1 1.0.0.1")
foreach(v IN LISTS LESSV)
  string(REGEX MATCH "(.*) (.*)" _dummy "${v}")
  # check less
  if(CMAKE_MATCH_1 VERSION_LESS CMAKE_MATCH_2)
    message(STATUS "${CMAKE_MATCH_1} is less than ${CMAKE_MATCH_2}")
  else()
    message(FATAL_ERROR "${CMAKE_MATCH_1} is not less than ${CMAKE_MATCH_2}?")
  endif()

  # check greater
  if(CMAKE_MATCH_2 VERSION_GREATER CMAKE_MATCH_1)
    message(STATUS "${CMAKE_MATCH_2} is greater than ${CMAKE_MATCH_1}")
  else()
    message(FATAL_ERROR "${CMAKE_MATCH_2} is not greater than ${CMAKE_MATCH_1}?")
  endif()

  # check less negative case
  if(NOT CMAKE_MATCH_2 VERSION_LESS CMAKE_MATCH_1)
    message(STATUS "${CMAKE_MATCH_2} is not less than ${CMAKE_MATCH_1}")
  else()
    message(FATAL_ERROR "${CMAKE_MATCH_2} is less than ${CMAKE_MATCH_1}?")
  endif()

  # check greater or equal (same as less negative)
  if(CMAKE_MATCH_2 VERSION_GREATER_EQUAL CMAKE_MATCH_1)
    message(STATUS "${CMAKE_MATCH_2} is not less than ${CMAKE_MATCH_1}")
  else()
    message(FATAL_ERROR "${CMAKE_MATCH_2} is less than ${CMAKE_MATCH_1}?")
  endif()

  # check greater negative case
  if(NOT CMAKE_MATCH_1 VERSION_GREATER CMAKE_MATCH_2)
    message(STATUS "${CMAKE_MATCH_1} is not greater than ${CMAKE_MATCH_2}")
  else()
    message(FATAL_ERROR "${CMAKE_MATCH_1} is greater than ${CMAKE_MATCH_2}?")
  endif()

  # check less or equal (same as greater negative) case
  if(CMAKE_MATCH_1 VERSION_LESS_EQUAL CMAKE_MATCH_2)
    message(STATUS "${CMAKE_MATCH_1} is not greater than ${CMAKE_MATCH_2}")
  else()
    message(FATAL_ERROR "${CMAKE_MATCH_1} is greater than ${CMAKE_MATCH_2}?")
  endif()
endforeach()
