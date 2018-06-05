project(TuriCreateObjC)

if(APPLE)
  set(_SDK_NAME "macosx")
  if(TC_BUILD_IOS)
    set(_SDK_NAME "iphoneos")
  endif()

  execute_process(
    COMMAND bash -c "xcrun --sdk ${_SDK_NAME} --show-sdk-path" 
    OUTPUT_VARIABLE _SDK_PATH 
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  set(_FRAMEWORK_SEARCH_PATH "${_SDK_PATH}/System/Library/Frameworks/;/System/Library/Frameworks/")

  find_library(FOUNDATION NAMES Foundation
    REQUIRED PATHS ${_FRAMEWORK_SEARCH_PATH} NO_DEFAULT_PATH)
  message("Foundation found at ${FOUNDATION}.")

  find_library(CORE_ML NAMES CoreML
    REQUIRED PATHS ${_FRAMEWORK_SEARCH_PATH} NO_DEFAULT_PATH)
  message("CoreML found at ${CORE_ML}.")

  make_library(Recommender
    SOURCES
      TuriCreateObjC.m
      recommender_initialization.cpp
    REQUIRES
      unity_recsys
      capi
      ${FOUNDATION}
      ${CORE_ML}
    SHARED
    SHARED_ALL_DEFINED
    EXPORT_OSX_MAP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/symbol_exports.ver
    DEAD_STRIP
  )

  make_library(objcapi
    SOURCES
      TuriCreateObjC.m
    REQUIRES
      TuriCreate_static
      ${FOUNDATION}
      ${CORE_ML}
  )

  target_compile_options(objcapi PUBLIC "-fobjc-arc")
  target_compile_options(Recommender PUBLIC "-fobjc-arc")

  if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_command(TARGET Recommender
      POST_BUILD
      COMMAND strip -Sx "$<TARGET_FILE:Recommender>"
      COMMENT "Stripping Objective-C Recommender library"
    )
  endif()

endif()
