enable_language(C)

set(vars
  CMAKE_EXECUTABLE_FORMAT
  )

if(CMAKE_HOST_UNIX)
  list(APPEND vars
    CMAKE_UNAME
    )
endif()

foreach(v IN LISTS vars)
  if(NOT DEFINED ${v})
    message(SEND_ERROR "Variable '${v}' is not set!")
  endif()
endforeach()
