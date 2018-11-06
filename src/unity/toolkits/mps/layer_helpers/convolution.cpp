#include <unity/toolkits/mps/layer_helpers/convolution.h>

namespace turi{
    namespace mps {
        ConvolutionNode::ConvolutionNode(std::string name,
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

        void ConvolutionNode::init(std::string name,
                                   std::shared_ptr<Layer> input,
                                   std::map<std::string, int> options,
                                   std::map<std::string, std::vector<float>> data) {

            m_input = input;

            m_kernel_width = options.find("kernel_width")->second;
            m_kernel_height = options.find("kernel_height")->second;
            
            m_input_feature_channels = options.find("input_feature_channels")->second;
            m_output_feature_channels = options.find("output_feature_channels")->second;

            m_stride_width = options.find("stride_width")->second;
            m_stride_height = options.find("stride_height")->second;

            m_padding_width = options.find("padding_width")->second;
            m_padding_height = options.find("padding_height")->second;

            m_weights = data.find("weights")->second;
            m_biases = data.find("biases")->second;

            m_name = name;
            m_type = layer_type::convolution;
        }
    }
}



