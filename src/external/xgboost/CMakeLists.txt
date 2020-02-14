project(Turi)

if(TC_BUILD_IOS)
  make_library(
    xgboost
    SOURCES
    src/gbm/gbm.cpp
    src/tree/updater.cpp
    src/io/dmlc_simple.cpp
    src/io/io.cpp
    subtree/rabit/src/engine_empty.cc
    REQUIRES
    # libjson
    boost)
else()
  make_library(
    xgboost
    SOURCES
    src/gbm/gbm.cpp
    src/tree/updater.cpp
    src/io/dmlc_simple.cpp
    src/io/io.cpp
    subtree/rabit/src/engine_empty.cc
    REQUIRES
    # libjson
    boost
    omp)
endif()

if(NOT TC_BUILD_IOS)
  target_link_libraries(xgboost PRIVATE omp)
  target_compile_options(xgboost PRIVATE "-Xpreprocessor -fopenmp")
endif()

target_compile_options(xgboost PUBLIC "-Wno-narrowing -msse2 -funroll-loops")
target_compile_definitions(xgboost PUBLIC "-DXGBOOST_CUSTOMIZE_MSG_")
