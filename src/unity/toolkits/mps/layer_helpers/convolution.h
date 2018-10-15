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
			std::shared_ptr<Layer> m_input;
			size_t m_kernelWidth;
			size_t m_kernelHeight;
			size_t m_inputFeatureChannels;
			size_t m_outputFeatureChannels;
			size_t m_strideWidth;
			size_t m_strideHeight;
			size_t m_paddingWidth;
			size_t m_paddingHeight;
			std::vector<float> m_weights;
			std::vector<float> m_biases;

			ConvolutionNode(){};
			ConvolutionNode(std::string name,
							std::shared_ptr<Layer> input,
							size_t kernelWidth,
							size_t kernelHeight,
							size_t inputFeatureChannels,
							size_t outputFeatureChannels,
							size_t strideWidth,
							size_t strideHeight,
							size_t paddingWidth,
							size_t paddingHeight,
							std::vector<float> weights,
							std::vector<float> biases):
				Layer(name, layer_type::addition),
				m_input(input),
				m_kernelWidth(kernelWidth),
				m_kernelHeight(kernelHeight),
				m_inputFeatureChannels(inputFeatureChannels),
				m_outputFeatureChannels(outputFeatureChannels),
				m_strideWidth(strideWidth),
				m_strideHeight(strideHeight),
				m_paddingWidth(paddingWidth),
				m_paddingHeight(paddingHeight),
				m_weights(weights),
				m_biases(biases) {};

			BEGIN_CLASS_MEMBER_REGISTRATION("_ConvolutionNode")
			END_CLASS_MEMBER_REGISTRATION
		};
	}
}

#endif