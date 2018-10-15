#ifndef turi_mps_layer_helpers_types_h
#define turi_mps_layer_helpers_types_h

namespace turi{
	namespace mps {
		enum layer_type {
			input,
			output,
			addition,
			convolution,
			instance_norm,
			pooling,
			relu,
			sigmoid,
			upsampling
		};
	}
}

#endif