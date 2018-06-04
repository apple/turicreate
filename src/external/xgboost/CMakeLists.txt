project(Turi)

make_library(xgboost
  SOURCES
    src/gbm/gbm.cpp
    src/tree/updater.cpp
    src/io/dmlc_simple.cpp
    src/io/io.cpp
    subtree/rabit/src/engine_empty.cc
  REQUIRES
    libjson
  EXTERNAL_VISIBILITY
)

# TODO this doesn't seem to take effect at all
set_property(TARGET xgboost APPEND_STRING PROPERTY COMPILE_FLAGS " -DXGBOOST_CUSTOMIZE_MSG_ -Wno-narrowing -msse2 -funroll-loops")
