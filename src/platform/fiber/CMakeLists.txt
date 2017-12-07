project(fiber)

make_library(fiber
  SOURCES
    fiber_control.cpp
    fiber_group.cpp
  REQUIRES
    parallel logger random
    EXTERNAL_VISIBILITY
)
