project(shmipc)

add_library(shmipc OBJECT
  shmipc.cpp
  shmipc_garbage_collect.cpp
)
add_dependencies(shmipc boost)