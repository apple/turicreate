project(unity_toolkits)

add_library(unity_image_export OBJECT
  image_fn_export.cpp
)
add_dependencies(unity_image_export boost)
