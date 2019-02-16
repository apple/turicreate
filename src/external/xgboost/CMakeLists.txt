project(Turi)

make_library(xgboost OBJECT
  SOURCES
    src/gbm/gbm.cpp
    src/tree/updater.cpp
    src/io/dmlc_simple.cpp
    src/io/io.cpp
    subtree/rabit/src/engine_empty.cc
  REQUIRES
    libjson
)

target_compile_options(xgboost PUBLIC "-Wno-narrowing -msse2 -funroll-loops")
target_compile_definitions(xgboost PUBLIC "-DXGBOOST_CUSTOMIZE_MSG_")
