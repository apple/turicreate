project(unity)

set(VISUALIZATION_SOURCES
  visualization/boxes_and_whiskers.cpp
  visualization/categorical_heatmap.cpp
  visualization/groupby.cpp
  visualization/heatmap.cpp
  visualization/histogram.cpp
  visualization/hyperloglog.cpp
  visualization/io_buffer.cpp
  visualization/item_frequency.cpp
  visualization/process_wrapper.cpp
  visualization/scatter.cpp
  visualization/show.cpp
  visualization/thread.cpp
  visualization/transformation.cpp
  visualization/vega_data.cpp
  visualization/vega_spec.cpp
)

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
    ${VISUALIZATION_SOURCES}
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
    EXTERNAL_VISIBILITY
)

set(GENERATED_VEGA_JSON_HEADERS
  vega_spec/boxes_and_whiskers.json
  vega_spec/categorical.json
  vega_spec/categorical_heatmap.json
  vega_spec/heatmap.json
  vega_spec/histogram.json
  vega_spec/scatter.json
  vega_spec/summary_view.json
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/visualization/vega_spec)

foreach(SOURCE_FILE ${GENERATED_VEGA_JSON_HEADERS})

  string(
    REGEX REPLACE
    ".json$"
    ".h"
    GENERATED_FILE
    ${CMAKE_CURRENT_BINARY_DIR}/visualization/${SOURCE_FILE}
  )

  add_custom_command(
    OUTPUT ${GENERATED_FILE}
    COMMAND xxd -i ${SOURCE_FILE} > ${GENERATED_FILE}
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/visualization/${SOURCE_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/visualization
    COMMENT "Generating ${GENERATED_FILE}"
    VERBATIM
  )
  
  string(
    MAKE_C_IDENTIFIER
    ${SOURCE_FILE}
    TARGET_NAME
  )

  add_custom_target(
    Generate_${TARGET_NAME}
    DEPENDS
    ${GENERATED_FILE}
  )

  add_dependencies(unity_core Generate_${TARGET_NAME})

endforeach(SOURCE_FILE)


make_shared_library_from_static(unity_core_shared
                                REQUIRES
                                unity_core)
