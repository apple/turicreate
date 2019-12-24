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
    boost
)

target_compile_options(xgboost PUBLIC "-Wno-narrowing -funroll-loops")
target_compile_definitions(xgboost PUBLIC "-DXGBOOST_CUSTOMIZE_MSG_")
