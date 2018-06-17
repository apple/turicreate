project(unity_toolkits)

if(APPLE AND HAS_MPS AND NOT TC_BUILD_IOS)
    # We need to get the proper SDK paths in the build
    execute_process(
      COMMAND bash -c "xcrun --sdk macosx --show-sdk-path"
      OUTPUT_VARIABLE _macosx_sdk_path
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    message("MacOSX SDK path: ${_macosx_sdk_path}")

    set(_FRAMEWORK_SEARCH_PATH "${_macosx_sdk_path}/System/Library/Frameworks/;/System/Library/Frameworks/")

    message("Framework library search paths: ${_FRAMEWORK_SEARCH_PATH}")

    find_library(FOUNDATION NAMES Foundation
      REQUIRED PATHS ${_FRAMEWORK_SEARCH_PATH} NO_DEFAULT_PATH)
    message("Foundation found at ${FOUNDATION}.")

    find_library(METAL NAMES Metal
      REQUIRED PATHS ${_FRAMEWORK_SEARCH_PATH} NO_DEFAULT_PATH)
    message("Metal found at ${METAL}.")

    find_library(MPS NAMES MetalPerformanceShaders
      REQUIRED PATHS ${_FRAMEWORK_SEARCH_PATH} NO_DEFAULT_PATH)
    message("MetalPerformanceShaders found at ${MPS}.")

    find_library(ACCELERATE NAMES Accelerate
      REQUIRED PATHS ${_FRAMEWORK_SEARCH_PATH} NO_DEFAULT_PATH)
    message("Accelerate found at ${ACCELERATE}.")

    make_library(tcmps
        SOURCES
        mps_graph_trainer.mm
        mps_graph_networks.mm
        mps_updater.mm
        mps_utils.mm
        mps_graph_cnnmodule.mm
        mps_layers.mm
        mps_networks.mm
        mps_trainer.mm
        mps_lstm_helper.mm
        mps_cnnmodule.mm
        mps_graph_layers.mm
        mps_weight.mm
        mps_dev.mm
        REQUIRES
          fileio
          objc
          ${FOUNDATION}
          ${METAL}
          ${MPS}
          ${ACCELERATE}
        SHARED
    )
    target_compile_options(tcmps PUBLIC "-fobjc-arc -Wno-nullability-completeness -Wno-shadow-ivar")
endif()
