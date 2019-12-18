project(Turi)

make_library(unity_prototype OBJECT
  SOURCES
    sparse_nn.cpp
    class_registrations.cpp
  REQUIRES
    unity_core
)
