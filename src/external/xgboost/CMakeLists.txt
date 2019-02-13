project(Turi)

add_library(xgboost OBJECT
  src/gbm/gbm.cpp
  src/tree/updater.cpp
  src/io/dmlc_simple.cpp
  src/io/io.cpp
  subtree/rabit/src/engine_empty.cc
)
add_dependencies(xgboost boost)

target_compile_options(xgboost PUBLIC "-Wno-narrowing -msse2 -funroll-loops")
target_compile_definitions(xgboost PUBLIC "-DXGBOOST_CUSTOMIZE_MSG_")