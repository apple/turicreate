project(TuriCreate)

# Fix the current source directory of the framework stuff so the macro below
# references everything correctly.
set(__CAPI_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "")

add_library(capi OBJECT
  "impl/capi_error_handling.cpp"
  "impl/capi_flexible_type.cpp"
  "impl/capi_datetime.cpp"
  "impl/capi_flex_dict.cpp"
  "impl/capi_flex_list.cpp"
  "impl/capi_flex_image.cpp"
  "impl/capi_ndarray.cpp"
  "impl/capi_functions.cpp"
  "impl/capi_sarray.cpp"
  "impl/capi_parameters.cpp"
  "impl/capi_sframe.cpp"
  "impl/capi_models.cpp"
  "impl/capi_functions.cpp"
  "impl/capi_initialization.cpp"
  "impl/capi_variant.cpp"
  "impl/capi_operators.cpp"
  "impl/capi_flex_enum_list.cpp"
  "impl/capi_wrapper_structs.cpp"
  "impl/capi_memory_management.cpp"
  "impl/capi_visualization.cpp"
)
add_dependencies(capi boost)

# This needs to be an object type library to avoid multiple compilations.
# (Most of the other OBJECT library targets in the repo are just to avoid
#  multiple linkings from a single compilation of each translation unit.)

set(_capi_objects
  "$<TARGET_OBJECTS:capi>"

  # Also list all dependencies (including transitive dependencies)

  # External dependencies (built from src/external)
  "$<TARGET_OBJECTS:aws-sdk-cpp>"
  "$<TARGET_OBJECTS:libjson>"
  "$<TARGET_OBJECTS:lz4>"
  "$<TARGET_OBJECTS:protobuf>"
  "$<TARGET_OBJECTS:uuid>"
  "$<TARGET_OBJECTS:xgboost>"

  # Then our own source code
  "$<TARGET_OBJECTS:cancel_serverside_ops>"
  "$<TARGET_OBJECTS:cppipc>"
  "$<TARGET_OBJECTS:flexible_type>"
  "$<TARGET_OBJECTS:globals>"
  "$<TARGET_OBJECTS:fileio>"
  "$<TARGET_OBJECTS:image_io>"
  "$<TARGET_OBJECTS:image_type>"
  "$<TARGET_OBJECTS:logger>"
  "$<TARGET_OBJECTS:minipsutil>"
  "$<TARGET_OBJECTS:ml_data>"
  "$<TARGET_OBJECTS:nanosockets>"
  "$<TARGET_OBJECTS:network>"
  "$<TARGET_OBJECTS:optimization>"
  "$<TARGET_OBJECTS:parallel>"
  "$<TARGET_OBJECTS:process>"
  "$<TARGET_OBJECTS:pylambda>"
  "$<TARGET_OBJECTS:random>"
  "$<TARGET_OBJECTS:serialization>"
  "$<TARGET_OBJECTS:sframe>"
  "$<TARGET_OBJECTS:sframe_query_engine>"
  "$<TARGET_OBJECTS:sgraph>"
  "$<TARGET_OBJECTS:shmipc>"
  "$<TARGET_OBJECTS:stack_trace>"
  "$<TARGET_OBJECTS:startup_teardown>"
  "$<TARGET_OBJECTS:supervised_learning>"
  "$<TARGET_OBJECTS:table_printer>"
  "$<TARGET_OBJECTS:timer>"
  "$<TARGET_OBJECTS:unity>"
  "$<TARGET_OBJECTS:unity_activity_classification>"
  "$<TARGET_OBJECTS:unity_core>"
  "$<TARGET_OBJECTS:unity_coreml_model_export>"
  "$<TARGET_OBJECTS:unity_clustering>"
  "$<TARGET_OBJECTS:unity_evaluation>"
  "$<TARGET_OBJECTS:unity_factorization>"
  "$<TARGET_OBJECTS:unity_feature_engineering>"
  "$<TARGET_OBJECTS:unity_graph_analytics>"
  "$<TARGET_OBJECTS:unity_image_export>"
  "$<TARGET_OBJECTS:unity_ml_data_2>"
  "$<TARGET_OBJECTS:unity_ml_model>"
  "$<TARGET_OBJECTS:unity_nearest_neighbors>"
  "$<TARGET_OBJECTS:unity_neural_net>"
  "$<TARGET_OBJECTS:unity_object_detection>"
  "$<TARGET_OBJECTS:unity_pattern_mining>"
  "$<TARGET_OBJECTS:unity_prototype>"
  "$<TARGET_OBJECTS:unity_recsys>"
  "$<TARGET_OBJECTS:unity_sgd>"
  "$<TARGET_OBJECTS:unity_sparse_similarity>"
  "$<TARGET_OBJECTS:unity_text>"
  "$<TARGET_OBJECTS:unity_util>"
  "$<TARGET_OBJECTS:util>"
  "$<TARGET_OBJECTS:visualization>"
)

set(_capi_requires
  # External dependencies (from deps)
  armadillo
  boost
  curl
  libjpeg
  libpng
  openssl
  nanomsg
)

if(APPLE)
  set(_capi_objects
    ${_capi_objects}
    "$<TARGET_OBJECTS:image_deep_feature_extractor>"
    "$<TARGET_OBJECTS:platform_config>"
  )
  set(_capi_requires
    ${_capi_requires}
    ${ACCELERATE}
    ${CORE_GRAPHICS}
    ${CORE_IMAGE}
    ${CORE_ML}
    ${CORE_VIDEO}
    ${FOUNDATION}
    ${JAVASCRIPTCORE}
    ${METAL}
    ${METAL_PERFORMANCE_SHADERS}
  )
  if(NOT TC_BUILD_IOS)
    set(_capi_objects
      ${_capi_objects}
      "$<TARGET_OBJECTS:tcmps>"
      "$<TARGET_OBJECTS:vega_renderer>"
    )
    set(_capi_requires
      ${_capi_requires}
      ${APPKIT}
    )
  endif()
endif()

# Create a static version of the library that also bundles in the server initializer
make_library(TuriCreate_static
  SOURCES
    default_server_initializer.cpp
    ${_capi_objects}
  REQUIRES
    ${_capi_requires}
)

# A macro that allows building a custom C-API library.
# 
#  The main variable here is the initializer source file and whether or not to 
#  build it as a framework. 
#
#  To build it with a custom server initialization function, which allows for 
#  specifying which models are availalable for calling through the C-API, call 
#
#  make_capi_library(MyTuriExecutor INITIALIZER_SOURCE_FILE my_turi_model_init.cpp REQUIRES unity)
#
#  See default_server_initializer.cpp as a template for this. 
#
macro(make_capi_library NAME)

  set(options FRAMEWORK)
  set(one_value_args INITIALIZER_SOURCE_FILE FRAMEWORK_IDENTIFIER REQUIRES)

  CMAKE_PARSE_ARGUMENTS(__capi "${options}" "${one_value_args}" "" ${ARGN})

  if(NOT __capi_INITIALIZER_SOURCE_FILE)
    set(__capi_INITIALIZER_SOURCE_FILE "${__CAPI_SOURCE_DIR}/default_server_initializer.cpp")
  endif()

  if(TC_BUILD_IOS)
    set(ios_option "--iOS")
  endif()

  if(${__capi_FRAMEWORK})

    set(FRAMEWORK_PUBLIC_HEADER "TuriCreate.h")

    make_library(${NAME}
      SOURCES
        ${__capi_INITIALIZER_SOURCE_FILE}
        ${_capi_objects}
       REQUIRES
       ${EXTRA_REQUIRES}
       ${__capi_REQUIRES}
      SHARED_ALL_DEFINED
      SHARED
      EXPORT_LINUX_MAP_FILE "${__CAPI_SOURCE_DIR}/framework/tc_capi_exports_linux.ver"
      EXPORT_OSX_MAP_FILE "${__CAPI_SOURCE_DIR}/framework/tc_capi_exports_osx.ver"
      DEAD_STRIP
    )

    set_target_properties(${NAME} PROPERTIES
      FRAMEWORK TRUE
      FRAMEWORK_VERSION A
      MACOSX_FRAMEWORK_IDENTIFIER "${__capi_FRAMEWORK_IDENTIFIER}"
      MACOSX_FRAMEWORK_INFO_PLIST "${__CAPI_SOURCE_DIR}/framework/Info.plist"
      PUBLIC_HEADER ${FRAMEWORK_PUBLIC_HEADER}
      MACOSX_RPATH TRUE
    )

    add_custom_command(TARGET ${NAME}
    POST_BUILD
      COMMAND "${__CAPI_SOURCE_DIR}/framework/process_framework.sh"
      --name ${NAME}
      --src-headers "${__CAPI_SOURCE_DIR}"
      --framework "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.framework"
      --install-location "@rpath"
      --create-tbd-file
      ${ios_option}
    BYPRODUCTS
      "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.framework/Versions/Current/Header/TuriCreate.h"
      "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.framework/Versions/Current/TuriCreate.tbd"
    )

    add_dependencies(${NAME} "${__CAPI_SOURCE_DIR}/framework/process_framework.sh")
    add_dependencies(${NAME} "${__CAPI_SOURCE_DIR}/framework/Info.plist")

  else()
    #build out as a regular library
        make_library(${NAME}
      SOURCES 
            ${__capi_INITIALIZER_SOURCE_FILE}
            ${_capi_objects}
       REQUIRES
            ${EXTRA_REQUIRES}
            ${__capi_REQUIRES}
            ${_capi_requires}
      SHARED_ALL_DEFINED
      SHARED
      EXPORT_LINUX_MAP_FILE "${__CAPI_SOURCE_DIR}/framework/tc_capi_exports_linux.ver"
      EXPORT_OSX_MAP_FILE "${__CAPI_SOURCE_DIR}/framework/tc_capi_exports_osx.ver"
      DEAD_STRIP
    )

  endif()

  # For both framework and non-framework, add .ver files as dependencies
  add_dependencies(${NAME} "${__CAPI_SOURCE_DIR}/framework/tc_capi_exports_osx.ver")
  add_dependencies(${NAME} "${__CAPI_SOURCE_DIR}/framework/tc_capi_exports_linux.ver")

endmacro()

if(${TC_BUILD_CAPI_FRAMEWORK})
  make_capi_library(TuriCreate
    FRAMEWORK
    FRAMEWORK_IDENTIFIER TuriCreate
  )
else()
  make_capi_library(TuriCreate)
endif()

