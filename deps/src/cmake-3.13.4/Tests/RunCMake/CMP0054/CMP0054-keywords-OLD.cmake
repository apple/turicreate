cmake_policy(SET CMP0054 OLD)

if(NOT 1)
  message(FATAL_ERROR "[NOT 1] evaluated true")
endif()

if("NOT" 1)
  message(FATAL_ERROR "[\"NOT\" 1] evaluated true")
endif()
