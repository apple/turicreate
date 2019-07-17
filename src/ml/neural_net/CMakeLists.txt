project(unity_toolkits)

# The unity_neural_net library provides a platform-agnostic interface to
# neural-network inference and training backends. It provides a factory method
# that should instantiate a backend appropriate to the current platform and
# hardware.

# When building for macOS, build the MetalPerformanceShaders-based backend.
if(APPLE AND HAS_MPS AND NOT TC_BUILD_IOS)

  make_library(tcmps OBJECT
    SOURCES
      TCMPSImageAugmenting.m
      mps_image_augmentation.mm
      mps_compute_context.mm
      mps_graph_trainer.mm
      mps_graph_networks.mm
      mps_updater.mm
      mps_utils.mm
      mps_graph_cnnmodule.mm
      mps_layers.mm
      mps_layer_helper.mm
      mps_layer_instance_norm_data_loader.mm
      mps_layer_conv_padding.m
      mps_networks.mm
      mps_trainer.mm
      mps_lstm_helper.mm
      mps_cnnmodule.mm
      mps_graph_layers.mm
      mps_weight.mm
      mps_device_manager.m
      mps_descriptor_utils.m
      style_transfer/mps_style_transfer_encoding_node.m
      style_transfer/mps_style_transfer_residual_node.m
      style_transfer/mps_style_transfer_decoding_node.m
      style_transfer/mps_style_transfer_transformer_network.m
      style_transfer/mps_style_transfer_vgg_16_block_1_node.m
      style_transfer/mps_style_transfer_vgg_16_block_2_node.m
      style_transfer/mps_style_transfer_vgg_16_network.m
    REQUIRES
      unity_core
      ${ACCELERATE}
      ${CORE_GRAPHICS}
      ${CORE_IMAGE}
      ${METAL}
      ${METAL_PERFORMANCE_SHADERS}
  )
  target_compile_options(tcmps PUBLIC "-fobjc-arc -Wno-nullability-completeness -Wno-shadow-ivar")
  set(additional_unity_neural_net_requirements tcmps)
else()
  set(additional_unity_neural_net_requirements "")
endif()

# TODO: a backend wrapping MXNet.

make_library(unity_neural_net OBJECT
  SOURCES
    compute_context.cpp
    float_array.cpp
    image_augmentation.cpp
    model_spec.cpp
    weight_init.cpp
  REQUIRES
    image_type
    logger
    unity_core
    unity_coreml_model_export
    ${additional_unity_neural_net_requirements}
)
