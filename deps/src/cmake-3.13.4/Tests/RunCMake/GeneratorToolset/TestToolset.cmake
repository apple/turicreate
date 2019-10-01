if("x${CMAKE_GENERATOR_TOOLSET}" STREQUAL "xTest Toolset")
  message(FATAL_ERROR "CMAKE_GENERATOR_TOOLSET is \"Test Toolset\" as expected.")
else()
  message(FATAL_ERROR
    "CMAKE_GENERATOR_TOOLSET is \"${CMAKE_GENERATOR_TOOLSET}\" "
    "but should be \"Test Toolset\"!")
endif()
