if("x${CMAKE_GENERATOR_PLATFORM}" STREQUAL "x")
  message(FATAL_ERROR "CMAKE_GENERATOR_PLATFORM is empty as expected.")
else()
  message(FATAL_ERROR
    "CMAKE_GENERATOR_PLATFORM is \"${CMAKE_GENERATOR_PLATFORM}\" "
    "but should be empty!")
endif()
