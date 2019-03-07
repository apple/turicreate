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

  make_library(image_deep_feature_extractor OBJECT
    SOURCES
      mlmodel_image_feature_extractor.mm
      class_registrations.cpp
      image_deep_feature_extractor_toolkit.cpp
    REQUIRES
      fileio
      objc
      coremltools_mlmodel
      ${CORE_ML}
      ${FOUNDATION}
      ${CORE_VIDEO}
    EXTERNAL_VISIBILITY
  )

endif()
