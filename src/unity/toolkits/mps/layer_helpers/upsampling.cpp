#include <unity/toolkits/mps/layer_helpers/upsampling.h>

namespace turi{
    namespace mps {
        UpsamplingNode::UpsamplingNode(std::string name,
                                       std::shared_ptr<Layer> input,
                                       size_t scale_x,
                                       size_t scale_y,
                                       size_t type):
            Layer(name, layer_type::upsampling),
            m_input(input),
            m_scale_x(scale_x),
            m_scale_y(scale_y),
            m_upsampling(static_cast<upsampling_type>(type)) {}

        void UpsamplingNode::init(std::string name,
                                  std::shared_ptr<Layer> input,
                                  size_t scale_x,
                                  size_t scale_y,
                                  size_t type) {
            m_input = input;
            m_scale_x = scale_x;
            m_scale_y = scale_y;
            m_upsampling = static_cast<upsampling_type>(type);

            m_name = name;
            m_type = layer_type::sigmoid;
        }
    }
}
