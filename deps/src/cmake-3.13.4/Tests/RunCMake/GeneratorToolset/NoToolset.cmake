if("x${CMAKE_GENERATOR_TOOLSET}" STREQUAL "x")
  message(FATAL_ERROR "CMAKE_GENERATOR_TOOLSET is empty as expected.")
else()
  message(FATAL_ERROR
    "CMAKE_GENERATOR_TOOLSET is \"${CMAKE_GENERATOR_TOOLSET}\" "
    "but should be empty!")
endif()
