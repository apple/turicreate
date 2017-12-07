if("x${CMAKE_GENERATOR_PLATFORM}" STREQUAL "xTest Platform")
  message(SEND_ERROR "CMAKE_GENERATOR_PLATFORM is \"Test Platform\" as expected.")
else()
  message(FATAL_ERROR
    "CMAKE_GENERATOR_PLATFORM is \"${CMAKE_GENERATOR_PLATFORM}\" "
    "but should be \"Test Platform\"!")
endif()
if(CMAKE_GENERATOR MATCHES "Visual Studio")
  if("x${CMAKE_VS_PLATFORM_NAME}" STREQUAL "xTest Platform")
    message(SEND_ERROR "CMAKE_VS_PLATFORM_NAME is \"Test Platform\" as expected.")
  else()
    message(FATAL_ERROR
      "CMAKE_VS_PLATFORM_NAME is \"${CMAKE_VS_PLATFORM_NAME}\" "
      "but should be \"Test Platform\"!")
  endif()
endif()
