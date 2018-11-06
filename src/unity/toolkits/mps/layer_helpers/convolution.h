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
                                int kernel_width,
                                int kernel_height,
                                int input_feature_channels,
                                int output_feature_channels,
                                int stride_width,
                                int stride_height,
                                int padding_width,
                                int padding_height,
                                std::vector<float> weights,
                                std::vector<float> biases);

                void init(std::string name,
                          std::shared_ptr<Layer> input,
                          std::map<std::string, int> options,
                          std::map<std::string, std::vector<float>> data);


                std::shared_ptr<Layer> m_input;
                int m_kernel_width;
                int m_kernel_height;
                int m_input_feature_channels;
                int m_output_feature_channels;
                int m_stride_width;
                int m_stride_height;
                int m_padding_width;
                int m_padding_height;
                std::vector<float> m_weights;
                std::vector<float> m_biases;


                BEGIN_CLASS_MEMBER_REGISTRATION("_ConvolutionNode")
                REGISTER_CLASS_MEMBER_FUNCTION(ConvolutionNode::init, "name", "input", "options", "data")
                END_CLASS_MEMBER_REGISTRATION
        };
    }
}

#endif