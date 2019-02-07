project(unity_toolkits)

add_library(unity_ml_model OBJECT
  python_model.cpp
)
add_dependencies(unity_ml_model boost)
