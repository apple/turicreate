#include <unity/toolkits/mps/layer_helpers/pooling.h>

namespace turi{
    namespace mps {
        PoolingNode::PoolingNode(std::string name,
                                 std::shared_ptr<Layer> input,
                                 size_t kernel_width,
                                 size_t kernel_height,
                                 size_t stride_in_pixels_x,
                                 size_t stride_in_pixels_y,
                                 size_t type):
            Layer(name, layer_type::upsampling),
            m_input(input),
            m_kernel_width(kernel_width),
            m_kernel_height(kernel_height),
            m_stride_in_pixels_x(stride_in_pixels_x),
            m_stride_in_pixels_y(stride_in_pixels_y),
            m_pooling(static_cast<pooling_type>(type)) {}

        void PoolingNode::init(std::string name,
                               std::shared_ptr<Layer> input,
                               std::pair<size_t, size_t> kernel,
                               std::pair<size_t, size_t> stride,
                               size_t type) {
            m_input = input;
            m_kernel_width = kernel.first;
            m_kernel_height = kernel.second;
            m_stride_in_pixels_x = stride.first;
            m_stride_in_pixels_y = stride.second;
            m_name = name;
            m_pooling = static_cast<pooling_type>(type);
            m_type = layer_type::pooling;
        }
    }
}
