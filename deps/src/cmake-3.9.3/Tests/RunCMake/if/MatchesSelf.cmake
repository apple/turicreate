foreach(n 0 1 2 3 4 5 6 7 8 9 COUNT)
  if(CMAKE_MATCH_${n} MATCHES "x")
  endif()
endforeach()
