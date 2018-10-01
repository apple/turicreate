project(TuriCreate)

# Fix the current source directory of the framework stuff so the macro below
# references everything correctly.
set(__CAPI_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "")

set(_turi_capi_source_files
  "${__CAPI_SOURCE_DIR}/impl/capi_error_handling.cpp"
  "${__CAPI_SOURCE_DIR}/impl/capi_flexible_type.cpp"
  "${__CAPI_SOURCE_DIR}/impl/capi_datetime.cpp"
  "${__CAPI_SOURCE_DIR}/impl/capi_flex_dict.cpp"
  "${__CAPI_SOURCE_DIR}/impl/capi_flex_list.cpp"
  "${__CAPI_SOURCE_DIR}/impl/capi_flex_image.cpp"
  "${__CAPI_SOURCE_DIR}/impl/capi_ndarray.cpp"
  "${__CAPI_SOURCE_DIR}/impl/capi_functions.cpp"
  "${__CAPI_SOURCE_DIR}/impl/capi_sarray.cpp"
  "${__CAPI_SOURCE_DIR}/impl/capi_parameters.cpp"
  "${__CAPI_SOURCE_DIR}/impl/capi_sframe.cpp"
  "${__CAPI_SOURCE_DIR}/impl/capi_models.cpp"
  "${__CAPI_SOURCE_DIR}/impl/capi_functions.cpp"
  "${__CAPI_SOURCE_DIR}/impl/capi_initialization.cpp"
  "${__CAPI_SOURCE_DIR}/impl/capi_variant.cpp"
  "${__CAPI_SOURCE_DIR}/impl/capi_operators.cpp"
  "${__CAPI_SOURCE_DIR}/impl/capi_flex_enum_list.cpp"
  "${__CAPI_SOURCE_DIR}/impl/capi_wrapper_structs.cpp"
  "${__CAPI_SOURCE_DIR}/impl/capi_memory_management.cpp"
  "${__CAPI_SOURCE_DIR}/impl/capi_visualization.cpp"
)

# This needs to be an object type library to avoid multiple compilations.
add_library(_capi_objects OBJECT
  ${_turi_capi_source_files}
)

set(_capi_requires unity unity_core numerics)
add_dependencies(_capi_objects ${_capi_requires})

make_library(capi 
  SOURCES
    "$<TARGET_OBJECTS:_capi_objects>"
  REQUIRES
  ${_capi_requires})
    
# Create a static version of the library that also bundles in the server initializer
make_library(TuriCreate_static
  SOURCES
    default_server_initializer.cpp
  REQUIRES
    capi
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
        $<TARGET_OBJECTS:_capi_objects>
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
            $<TARGET_OBJECTS:_capi_objects>
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

