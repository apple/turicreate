project(serialization)

add_library(serialization OBJECT
  dir_archive.cpp
)
add_dependencies(serialization boost)