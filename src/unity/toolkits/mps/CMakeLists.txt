project(unity_toolkits)

# load layer files
set(LAYERS
	layers/addition_layer.mm
	layers/average_pooling_layer.mm
	layers/convolution_data_loader.mm
	layers/convolution_layer.mm
	layers/convolution_padding.mm
	layers/instance_norm_data_loader.mm
	layers/instance_norm_layer.mm
	layers/nearest_upsampling_layer.mm
	layers/relu_layer.mm
	layers/sigmoid_layer.mm
)

# load mps files
set(MPS
	graph.mm
	utils.mm
)

# make the mps library
make_library(unity_mps_graph
  SOURCES
  	class_registrations.cpp
    ${MPS}
    ${LAYERS}
  REQUIRES
    unity_core
 )
