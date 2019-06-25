project(unity_toolkits)

make_library(unity_image_export OBJECT
  SOURCES
    image_fn_export.cpp
  REQUIRES
    unity_core
  EXTERNAL_VISIBILITY
)
