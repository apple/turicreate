
project(turi_capi)

set(capi_sources 
    impl/capi_error_handling.cpp
    impl/capi_flexible_type.cpp
    impl/capi_flex_dict.cpp
    impl/capi_flex_list.cpp
    impl/capi_flex_image.cpp
    impl/capi_sarray.cpp
    impl/capi_sketch.cpp
    impl/capi_parameters.cpp
    impl/capi_sframe.cpp
    impl/capi_models.cpp
    impl/capi_variant.cpp
    )

set(capi_dependencies unity numerics)

  # Internal version
make_library(turi_capi
  SOURCES
  ${capi_sources}
  REQUIRES
  ${capi_dependencies}
)

set(_build_framework 0)
if(APPLE)
  if(TC_BUILD_CAPI_FRAMEWORK)
    set(_build_framework 1)
  endif()
endif()


if(${_build_framework}) 

make_library(TuriCore
  SOURCES 
  ${capi_sources}
   REQUIRES
   ${capi_dependencies}
  SHARED_ALL_DEFINED
  SHARED
  EXPORT_LINUX_MAP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/tc_capi_exports_linux.ver"
  EXPORT_OSX_MAP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/tc_capi_exports_osx.ver"
  DEAD_STRIP
)

  set_target_properties(TuriCore PROPERTIES
    FRAMEWORK TRUE
    FRAMEWORK_VERSION A
    MACOSX_FRAMEWORK_IDENTIFIER com.apple.TuriCore
    MACOSX_FRAMEWORK_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/framework/Info.plist"
    PUBLIC_HEADER TuriCore.h)

  if(${TC_CAPI_AS_SYSTEM_FRAMEWORK})
    set(_install_location "/System/Library/PrivateFrameworks")
  else()
    set(_install_location "@rpath")
  endif()


  add_custom_command(TARGET TuriCore
    POST_BUILD
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/framework/process_framework.sh"
    --name TuriCore 
    --src-headers "${CMAKE_CURRENT_SOURCE_DIR}"
    --framework "${CMAKE_CURRENT_BINARY_DIR}/TuriCore.framework"
    --install-location "${_install_location}")

  add_dependencies(TuriCore "${CMAKE_CURRENT_SOURCE_DIR}/framework/process_framework.sh")


else()
  #build out as a regular library
  make_library(turi
  SOURCES 
  ${capi_sources}
   REQUIRES
   ${capi_dependencies}
     unity_core
    supervised_learning
  SHARED_ALL_DEFINED
  SHARED
  EXPORT_LINUX_MAP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/tc_capi_exports_linux.ver"
  EXPORT_OSX_MAP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/tc_capi_exports_osx.ver"
  DEAD_STRIP
)

endif()
