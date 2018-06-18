if(HAS_CORE_ML)
  project(image_deep_feature_extractor)

  find_library(core_ml NAMES CoreML)
  message("CoreML found at ${core_ml}.")

  find_library(core_foundation NAMES CoreFoundation)
  message("CoreFoundation found at ${core_foundation}.")

  find_library(foundation NAMES Foundation)
  message("Foundation found at ${foundation}.")

  find_library(core_video NAMES CoreVideo)
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
