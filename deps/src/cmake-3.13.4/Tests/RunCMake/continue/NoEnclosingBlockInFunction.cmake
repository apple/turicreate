function(foo)
  continue()
endfunction(foo)

foreach(i RANGE 1 2)
  foo()
  message(STATUS "Hello World")
endforeach()
