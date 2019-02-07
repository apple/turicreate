project(unity)

subdirs(visualization)

add_library(unity_core OBJECT
  api/function_closure_info.cpp
  extensions/model_base.cpp
  extensions/ml_model.cpp
  extensions/option_manager.cpp
  extensions/option_info.cpp
  variant_converter.cpp
  variant.cpp
  unity_global.cpp
  unity_global_singleton.cpp
  get_toolkit_function_from_closure.cpp
  toolkit_class_registry.cpp
  toolkit_function_registry.cpp
  simple_model.cpp
  variant_deep_serialize.cpp
  unity_sarray_binary_operations.cpp
  unity_sarray.cpp
  unity_sframe.cpp
  flex_dict_view.cpp
  unity_sgraph.cpp
  unity_sketch.cpp
  gl_datatype_registration.cpp
  gl_sgraph.cpp
  gl_gframe.cpp
  gl_sarray.cpp
  gl_sframe.cpp
  image_util.cpp
  ../extensions/additional_sframe_utilities.cpp
  unity_sarray_builder.cpp
  unity_sframe_builder.cpp
  visualization/columnwise_summary.cpp
)

add_dependencies(unity_core boost curl)