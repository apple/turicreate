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

		enum pooling_type {
			average,
			average_gradient,
			l_two_norm,
			max
		};

		enum upsampling_type {
			bilinear,
			bilinear_gradient,
			gradient,
			nearest
		};
	}
}

#endif