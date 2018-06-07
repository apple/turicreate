if(HAS_CORE_ML)
  project(image_deep_feature_extractor)

  # We need to get the proper SDK paths in the build
  execute_process(
    COMMAND bash -c "xcrun --sdk macosx --show-sdk-path"
    OUTPUT_VARIABLE _macosx_sdk_path
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  message("MacOSX SDK path: ${_macosx_sdk_path}")

  set(_FRAMEWORK_SEARCH_PATH "${_macosx_sdk_path}/System/Library/Frameworks/;/System/Library/Frameworks/")

  message("Framework library search paths: ${_FRAMEWORK_SEARCH_PATH}")

  find_library(core_ml NAMES CoreML
    REQUIRED PATHS ${_FRAMEWORK_SEARCH_PATH} NO_DEFAULT_PATH)
  message("CoreML found at ${core_ml}.")

  find_library(core_foundation NAMES CoreFoundation
    REQUIRED PATHS ${_FRAMEWORK_SEARCH_PATH} NO_DEFAULT_PATH)
  message("CoreFoundation found at ${core_foundation}.")

  find_library(foundation NAMES Foundation
    REQUIRED PATHS ${_FRAMEWORK_SEARCH_PATH} NO_DEFAULT_PATH)
  message("Foundation found at ${foundation}.")

  find_library(core_video NAMES CoreVideo
    REQUIRED PATHS ${_FRAMEWORK_SEARCH_PATH} NO_DEFAULT_PATH)
  message("CoreVideo found at ${core_video}.")

  make_library(image_deep_feature_extractor
        SOURCES
          mlmodel_image_feature_extractor.mm
          class_registrations.cpp
          image_deep_feature_extractor_toolkit.cpp
        REQUIRES
          fileio
          objc
          unity_coreml_model_export
          ${core_ml}
          ${core_foundation}
          ${foundation}
          ${core_video}
        EXTERNAL_VISIBILITY
  )

endif()
