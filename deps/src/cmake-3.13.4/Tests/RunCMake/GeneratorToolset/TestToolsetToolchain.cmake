if("x${CMAKE_GENERATOR_TOOLSET}" STREQUAL "xTest Toolset")
  message(SEND_ERROR "CMAKE_GENERATOR_TOOLSET is \"Test Toolset\" as expected.")
else()
  message(FATAL_ERROR
    "CMAKE_GENERATOR_TOOLSET is \"${CMAKE_GENERATOR_TOOLSET}\" "
    "but should be \"Test Toolset\"!")
endif()
if(CMAKE_GENERATOR MATCHES "Visual Studio")
  if("x${CMAKE_VS_PLATFORM_TOOLSET}" STREQUAL "xTest Toolset")
    message(SEND_ERROR "CMAKE_VS_PLATFORM_TOOLSET is \"Test Toolset\" as expected.")
  else()
    message(FATAL_ERROR
      "CMAKE_VS_PLATFORM_TOOLSET is \"${CMAKE_VS_PLATFORM_TOOLSET}\" "
      "but should be \"Test Toolset\"!")
  endif()
endif()
if(CMAKE_GENERATOR MATCHES "Xcode")
  if("x${CMAKE_XCODE_PLATFORM_TOOLSET}" STREQUAL "xTest Toolset")
    message(SEND_ERROR "CMAKE_XCODE_PLATFORM_TOOLSET is \"Test Toolset\" as expected.")
  else()
    message(FATAL_ERROR
      "CMAKE_XCODE_PLATFORM_TOOLSET is \"${CMAKE_XCODE_PLATFORM_TOOLSET}\" "
      "but should be \"Test Toolset\"!")
  endif()
endif()
