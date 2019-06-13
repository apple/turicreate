project(unity)

make_library(unity_core OBJECT
  SOURCES
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
    flex_dict_view.cpp
    image_util.cpp
    ../extensions/additional_sframe_utilities.cpp
  REQUIRES
    unity_sframe
    gl_sframe
    flexible_type
    pylambda
    table_printer
    sframe
    cancel_serverside_ops
    sframe_query_engine
    libjson sgraph
    curl
    image_type image_io
    startup_teardown
    pylambda_worker
    eigen
    visualization
    annotation
    EXTERNAL_VISIBILITY
)
