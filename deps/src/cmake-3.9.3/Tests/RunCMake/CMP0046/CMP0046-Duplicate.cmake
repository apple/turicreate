
add_library(dummy empty.cpp)

macro(add_dep)
  add_dependencies(dummy ctgt_no_exist)
endmacro()

add_dep()
add_dep()
