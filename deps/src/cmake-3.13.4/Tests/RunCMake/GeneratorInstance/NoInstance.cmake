if("x${CMAKE_GENERATOR_INSTANCE}" STREQUAL "x")
  message(FATAL_ERROR "CMAKE_GENERATOR_INSTANCE is empty as expected.")
else()
  message(FATAL_ERROR
    "CMAKE_GENERATOR_INSTANCE is \"${CMAKE_GENERATOR_INSTANCE}\" "
    "but should be empty!")
endif()
