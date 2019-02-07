project(unity_toolkits)

# The unity_neural_net library provides a platform-agnostic interface to
# neural-network inference and training backends. It provides a factory method
# that should instantiate a backend appropriate to the current platform and
# hardware.

# When building for macOS, build the MetalPerformanceShaders-based backend.
if(APPLE AND HAS_MPS AND NOT TC_BUILD_IOS)
    find_library(ACCELERATE NAMES Accelerate)
    message("Accelerate found at ${ACCELERATE}.")

    find_library(CORE_IMAGE NAMES CoreImage)
    message("CoreImage found at ${CORE_IMAGE}.")

    find_library(METAL NAMES Metal)
    message("Metal found at ${METAL}.")

    find_library(METAL_PERFORMANCE_SHADERS NAMES MetalPerformanceShaders)
    message("MetalPerformanceShaders found at ${METAL_PERFORMANCE_SHADERS}.")

    add_library(tcmps OBJECT
      TCMPSImageAugmenting.m
      mps_image_augmentation.mm
      mps_compute_context.mm
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
    )
    add_dependencies(tcmps boost)
    target_compile_options(tcmps PUBLIC "-fobjc-arc -Wno-nullability-completeness -Wno-shadow-ivar")
endif()

# TODO: a backend wrapping MXNet.

add_library(unity_neural_net OBJECT
  compute_context.cpp
  float_array.cpp
  image_augmentation.cpp
  model_spec.cpp
)
add_dependencies(unity_neural_net boost)
