#ifndef turi_mps_layer_helpers_convolution_h
#define turi_mps_layer_helpers_convolution_h

#include <unity/toolkits/mps/layer_helpers/types.h>
#include <unity/toolkits/mps/layer_helpers/base.h>

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/extensions/model_base.hpp>

#include <string>
#include <vector>
#include <map>

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
                                std::vector<float> biases);

                void init(std::string name,
                          std::shared_ptr<Layer> input,
                          std::map<std::string, size_t> options,
                          std::map<std::string, std::vector<float>> data);


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
                REGISTER_CLASS_MEMBER_FUNCTION(ConvolutionNode::init, "name", "input", "options", "data")
                END_CLASS_MEMBER_REGISTRATION
        };
    }
}

#endif