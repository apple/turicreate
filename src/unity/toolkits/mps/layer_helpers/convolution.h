#ifndef turi_mps_layer_helpers_convolution_h
#define turi_mps_layer_helpers_convolution_h

#include <unity/toolkits/mps/layer_helpers/types.h>
#include <unity/toolkits/mps/layer_helpers/base.h>

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/extensions/model_base.hpp>

#include <string>
#include <vector>

namespace turi{
	namespace mps {
		struct EXPORT ConvolutionNode: public Layer {
			public:
				ConvolutionNode(){};
				ConvolutionNode(std::string name,
								std::shared_ptr<Layer> input,
								size_t kernel_width,
								size_t kernel_height,
								size_t input_feature_channels,
								size_t output_feature_channels,
								size_t stride_width,
								size_t stride_height,
								size_t padding_width,
								size_t padding_height,
								std::vector<float> weights,
								std::vector<float> biases):
					Layer(name, layer_type::convolution),
					m_input(input),
					m_kernel_width(kernel_width),
					m_kernel_height(kernel_height),
					m_input_feature_channels(input_feature_channels),
					m_output_feature_channels(output_feature_channels),
					m_stride_width(stride_width),
					m_stride_height(stride_height),
					m_padding_width(padding_width),
					m_padding_height(padding_height),
					m_weights(weights),
					m_biases(biases) {};

				std::shared_ptr<Layer> m_input;
				size_t m_kernel_width;
				size_t m_kernel_height;
				size_t m_input_feature_channels;
				size_t m_output_feature_channels;
				size_t m_stride_width;
				size_t m_stride_height;
				size_t m_padding_width;
				size_t m_padding_height;
				std::vector<float> m_weights;
				std::vector<float> m_biases;


				BEGIN_CLASS_MEMBER_REGISTRATION("_ConvolutionNode")
				END_CLASS_MEMBER_REGISTRATION
		};
	}
}

#endif