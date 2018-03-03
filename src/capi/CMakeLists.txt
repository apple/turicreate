
project(turi_capi)

set(capi_sources 
    impl/capi_error_handling.cpp
    impl/capi_flexible_type.cpp
    impl/capi_datetime.cpp
    impl/capi_flex_dict.cpp
    impl/capi_flex_list.cpp
    impl/capi_flex_image.cpp
    impl/capi_ndarray.cpp
    impl/capi_functions.cpp
    impl/capi_sarray.cpp
    impl/capi_sketch.cpp
    impl/capi_parameters.cpp
    impl/capi_sframe.cpp
    impl/capi_models.cpp
    impl/capi_functions.cpp
    impl/capi_variant.cpp
    impl/capi_flex_enum_list.cpp
    impl/capi_wrapper_structs.cpp
    )

set(capi_dependencies unity numerics)

if(TC_CAPI_EXTERNAL_DEPENDENCIES)
  set(capi_dependencies ${capi_dependencies} ${TC_CAPI_EXTERNAL_DEPENDENCIES})
endif()

  # Internal version
make_library(turi_capi
  SOURCES
  ${capi_sources}
  REQUIRES
  ${capi_dependencies}
)


if(TC_CAPI_SERVER_INITIALIZER_CREATION_FUNCTION)
  message("Building C-API with ${TC_CAPI_SERVER_INITIALIZER_CREATION_FUNCTION} for server initializer class.")

  set_source_files_properties(impl/capi_models.cpp PROPERTIES COMPILE_FLAGS "-DTC_CAPI_SERVER_INITIALIZER_CREATION_FUNCTION=${TC_CAPI_SERVER_INITIALIZER_CREATION_FUNCTION}")
else()
  message("Building C-API with default unity server initializer.")
endif()


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

  set(_cmake_framework_flags "")

  if(TC_CAPI_FRAMEWORK_PATH)
    message("Building framework with path ${TC_CAPI_FRAMEWORK_PATH}.")
    set(_install_location "${TC_CAPI_FRAMEWORK_PATH}")
    set(_cmake_framework_flags "${_cmake_framework_flags} INSTALL_NAME_DIR \"${_install_location}\" BUILD_WITH_INSTALL_RPATH 0 MACOSX_RPATH" )

    set(CMAKE_INSTALL_NAME_DIR ${TC_CAPI_FRAMEWORK_PATH})

    set(CMAKE_SKIP_RPATH TRUE)

    set_target_properties(TuriCore PROPERTIES
      FRAMEWORK TRUE
      FRAMEWORK_VERSION A
      MACOSX_FRAMEWORK_IDENTIFIER com.apple.TuriCore
      MACOSX_FRAMEWORK_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/framework/Info.plist"
      INSTALL_NAME_DIR "${_install_location}"
      BUILD_WITH_INSTALL_NAME_DIR TRUE
      MACOSX_RPATH 0
      BUILD_WITH_INSTALL_RPATH 0
      PUBLIC_HEADER TuriCore.h
    )

  # Note: The install_name_tool has to fix the install_name_dir after this;
  # apparently the set(CMAKE_SKIP_RPATH TRUE) command above -- which is required to
  # remove the rpath in the framework -- doesn't really let the INSTALL_NAME_DIR
  # get set correctly.

  else()
    set(_install_location "@rpath")

    set_target_properties(TuriCore PROPERTIES
      FRAMEWORK TRUE
      FRAMEWORK_VERSION A
      MACOSX_FRAMEWORK_IDENTIFIER com.apple.TuriCore
      MACOSX_FRAMEWORK_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/framework/Info.plist"
      PUBLIC_HEADER TuriCore.h
      MACOSX_RPATH TRUE
    )
  endif()

  if(TC_CAPI_FRAMEWORK_DISABLE_TBD_FILE)
    message("Skipping in-build creation of TuriCore.tbd.")
    set(_tbd_options "")
  else()
    message("Creating in-build version of TuriCore.tbd.")
    set(_tbd_options "--create-tbd-file")
  endif()

  add_custom_command(TARGET TuriCore
    POST_BUILD
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/framework/process_framework.sh"
    --name TuriCore 
    --src-headers "${CMAKE_CURRENT_SOURCE_DIR}"
    --framework "${CMAKE_CURRENT_BINARY_DIR}/TuriCore.framework"
    --install-location "${_install_location}"
    ${_tbd_options}
    BYPRODUCTS
    "${CMAKE_CURRENT_BINARY_DIR}/TuriCore.framework/Versions/Current/Header/TuriCore.h"
    "${CMAKE_CURRENT_BINARY_DIR}/TuriCore.framework/Versions/Current/TuriCore.tbd"
    )

  if(TC_CAPI_FRAMEWORK_PATH)
    # After the previous command is run, run this one to fix the framework path in the library.
    add_custom_command(
      TARGET TuriCore
      POST_BUILD
      COMMAND install_name_tool $<TARGET_FILE:TuriCore> -id ${TC_CAPI_FRAMEWORK_PATH}/TuriCore.framework/Versions/A/TuriCore
    )
  endif()

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
