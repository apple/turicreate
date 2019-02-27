project(unity_toolkits)

make_library(unity_text OBJECT
  SOURCES
    unity_text.cpp
    topic_model.cpp
    cgs.cpp
    alias.cpp
    perplexity.cpp
    class_registrations.cpp
  REQUIRES
    unity_core
    unity_ml_model
    random
    logger
    unity_ml_data_2
    unity_util
)
