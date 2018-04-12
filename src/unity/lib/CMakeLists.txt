project(unity)

subdirs(visualization)

make_library(unity_core
  SOURCES
    api/function_closure_info.cpp
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
    toolkit_class_base.cpp
    gl_sgraph.cpp
    gl_gframe.cpp
    gl_sarray.cpp
    gl_sframe.cpp
    image_util.cpp
    ../extensions/additional_sframe_utilities.cpp
    unity_sarray_builder.cpp
    unity_sframe_builder.cpp
    visualization/columnwise_summary.cpp
  REQUIRES
    flexible_type
    pylambda 
    table_printer
    sframe cancel_serverside_ops 
    sframe_query_engine 
    libjson sgraph 
    curl 
    image_type image_io
    startup_teardown
    pylambda_worker
    numerics
    visualization
    EXTERNAL_VISIBILITY
)

make_shared_library_from_static(unity_core_shared
                                REQUIRES
                                unity_core)

