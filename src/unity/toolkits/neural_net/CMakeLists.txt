project(unity_toolkits)

if(APPLE AND HAS_MPS AND NOT TC_BUILD_IOS)
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
    )
    target_compile_options(tcmps PUBLIC "-fobjc-arc -Wno-nullability-completeness -Wno-shadow-ivar")
    set(additional_unity_neural_net_requirements tcmps)
else()
    set(additional_unity_neural_net_requirements "")
endif()

make_library(unity_neural_net
  SOURCES
    cnn_module.cpp
    float_array.cpp
  REQUIRES
    ${additional_unity_neural_net_requirements}
)
