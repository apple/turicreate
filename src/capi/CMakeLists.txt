project(TuriCreate)

# Internal version
make_library(_turi_capi_core
  SOURCES
    impl/capi_error_handling.cpp
    impl/capi_flexible_type.cpp
    impl/capi_datetime.cpp
    impl/capi_flex_dict.cpp
    impl/capi_flex_list.cpp
    impl/capi_flex_image.cpp
    impl/capi_ndarray.cpp
    impl/capi_functions.cpp
    impl/capi_sarray.cpp
    impl/capi_parameters.cpp
    impl/capi_sframe.cpp
    impl/capi_models.cpp
    impl/capi_functions.cpp
    impl/capi_initialization.cpp
    impl/capi_variant.cpp
    impl/capi_flex_enum_list.cpp
    impl/capi_wrapper_structs.cpp
    impl/capi_memory_management.cpp
  REQUIRES
    unity_core
    numerics)

make_library(_turi_capi_default_initializer
  SOURCES
    default_server_initializer.cpp
  REQUIRES
    unity)

# Create a static version of the library that also bundles in the server initializer
make_empty_library(TuriCreate_static
  REQUIRES
  unity _turi_capi_default_initializer _turi_capi_core)

# Fix the current source directory of the framework stuff so the macro below
# references everything correctly.
set(__CAPI_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "")

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

  if(${__capi_FRAMEWORK})

    make_library(${NAME}
      SOURCES
      ${__capi_INITIALIZER_SOURCE_FILE}
       REQUIRES
       _turi_capi_core
       ${__capi_REQUIRES}
      SHARED_ALL_DEFINED
      SHARED
      EXPORT_LINUX_MAP_FILE "${__CAPI_SOURCE_DIR}/framework/tc_capi_exports_linux.ver"
      EXPORT_OSX_MAP_FILE "${__CAPI_SOURCE_DIR}/framework/tc_capi_exports_osx.ver"
      DEAD_STRIP
    )

    set_target_properties(TuriCreate PROPERTIES
      FRAMEWORK TRUE
      FRAMEWORK_VERSION A
      MACOSX_FRAMEWORK_IDENTIFIER "${__capi_FRAMEWORK_IDENTIFIER}"
      MACOSX_FRAMEWORK_INFO_PLIST "${__CAPI_SOURCE_DIR}/framework/Info.plist"
      PUBLIC_HEADER TuriCreate.h
      MACOSX_RPATH TRUE
    )

    add_custom_command(TARGET TuriCreate
    POST_BUILD
      COMMAND "${__CAPI_SOURCE_DIR}/framework/process_framework.sh"
      --name ${NAME}
      --src-headers "${__CAPI_SOURCE_DIR}"
      --framework "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.framework"
      --install-location "@rpath"
      --create-tbd-file
    BYPRODUCTS
      "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.framework/Versions/Current/Header/TuriCreate.h"
      "${CMAKE_CURRENT_BINARY_DIR}/${NAME}.framework/Versions/Current/TuriCreate.tbd"
    )

    add_dependencies(${NAME} "${__CAPI_SOURCE_DIR}/framework/process_framework.sh")
    add_dependencies(${NAME} "${__CAPI_SOURCE_DIR}/framework/Info.plist")
    add_dependencies(${NAME} "${__CAPI_SOURCE_DIR}/framework/tc_capi_exports_osx.ver")
    add_dependencies(${NAME} "${__CAPI_SOURCE_DIR}/framework/tc_capi_exports_linux.ver")

else()
  #build out as a regular library
    make_library(${NAME}
  SOURCES 
        ${__capi_INITIALIZER_SOURCE_FILE}

   REQUIRES
         _turi_capi_core
         ${__capi_REQUIRES}
  SHARED_ALL_DEFINED
  SHARED
      EXPORT_LINUX_MAP_FILE "${__CAPI_SOURCE_DIR}/framework/tc_capi_exports_linux.ver"
      EXPORT_OSX_MAP_FILE "${__CAPI_SOURCE_DIR}/framework/tc_capi_exports_osx.ver"
  DEAD_STRIP
)

endif()

endmacro()

make_capi_library(TuriCreate)
