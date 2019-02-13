project(startup_teardown)

add_library(startup_teardown OBJECT
  startup_teardown.cpp
)
add_dependencies(startup_teardown boost)