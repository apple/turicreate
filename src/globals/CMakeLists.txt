project(globals)

add_library(globals OBJECT
  globals.cpp
)
add_dependencies(globals boost)