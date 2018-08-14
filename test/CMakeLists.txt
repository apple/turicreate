project(TuriTest)

enable_testing()

make_library(test_dependencies
  SOURCES
    ${CMAKE_SOURCE_DIR}/dummy.cpp
  REQUIRES
    TuriCreate_static
    capi
    cppipc
    crash_handler
    fileio
    flexible_type
    lazy_eval
    logger
    ml_data
    nanosockets
    network
    numerics
    optimization
    parallel
    process
    pylambda
    random
    serialization
    sframe
    sframe_query_engine
    sgraph
    shmipc
    sketches
    supervised_learning
    table_printer
    timer
    unity_core
    unity_clustering
    unity_evaluation
    unity_feature_engineering
    unity_ml_data_2
    unity_nearest_neighbors
    unity_pattern_mining
    unity_recsys
    unity_sparse_similarity
    unity_text
    unity_util
    user_pagefault
    util
    xgboost
  SHARED
  SHARED_ALL_DEFINED
  EXTERNAL_VISIBILITY
)

if(CLANG)
  set_target_properties(test_dependencies PROPERTIES
    LINK_FLAGS "-Wl,-all_load"
  )
else()
  set_target_properties(test_dependencies PROPERTIES
    LINK_FLAGS "-Wl,--whole-archive"
  )
endif()

subdirs(
        toolkits
        ml_data
        optimization
        user_pagefault
        util
        serialization
        logger
        timer
        random
        nanosockets
        fileio
        shmipc
        cppipc
        flexible_type
        parallel
        lambda
        sframe
        sgraph
        lazy_eval
        sframe_query_engine
        unity
        sketches
        generics
        table_printer
        network
        process
  )
if(${TC_BUILD_CAPI}) 
  message("Building C API Tests.")
  subdirs(capi)
else()
  message("Skipping build of C APIs Tests.")
endif()