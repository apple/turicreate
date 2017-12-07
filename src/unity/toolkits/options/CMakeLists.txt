project(unity_toolkits)

make_library(unity_options
  SOURCES
    option_manager.cpp option_info.cpp
  REQUIRES
    unity_core 
  EXTERNAL_VISIBILITY)
