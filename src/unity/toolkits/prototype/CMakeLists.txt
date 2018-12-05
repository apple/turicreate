project(unity_toolkits)

make_library(sparse_nn
  SOURCES
    sparse_nn.cpp
    class_registrations.cpp
  REQUIRES
    unity_core
)
