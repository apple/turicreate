project(unity_toolkits)

# The unity_neural_net library provides a platform-agnostic interface to
# neural-network inference and training backends. It provides a factory method
# that should instantiate a backend appropriate to the current platform and
# hardware.

# When building for macOS, build the MetalPerformanceShaders-based backend.
if(APPLE AND HAS_MPS AND NOT TC_BUILD_IOS)
    find_library(ACCELERATE NAMES Accelerate)
    message("Accelerate found at ${ACCELERATE}.")

    find_library(METAL NAMES Metal)
    message("Metal found at ${METAL}.")

    find_library(METAL_PERFORMANCE_SHADERS NAMES MetalPerformanceShaders)
    message("MetalPerformanceShaders found at ${METAL_PERFORMANCE_SHADERS}.")

    make_library(tcmps
        SOURCES
        mps_cnn_module_factory.mm
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
        mps_device_manager.m
      REQUIRES
        ${ACCELERATE}
        ${METAL}
        ${METAL_PERFORMANCE_SHADERS}
    )
    target_compile_options(tcmps PUBLIC "-fobjc-arc -Wno-nullability-completeness -Wno-shadow-ivar")
    set(additional_unity_neural_net_requirements tcmps)
else()
    set(additional_unity_neural_net_requirements "")
endif()

# TODO: a backend wrapping MXNet.

make_library(unity_neural_net
  SOURCES
    cnn_module.cpp
    coreml_import.cpp
    float_array.cpp
  REQUIRES
    logger
    unity_coreml_model_export
    ${additional_unity_neural_net_requirements}
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
    -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
)
