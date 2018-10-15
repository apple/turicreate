#include <unity/toolkits/mps/layer_helpers/sigmoid.h>

namespace turi{
    namespace mps {
        SigmoidNode::SigmoidNode(std::string name, std::shared_ptr<Layer> input):
            Layer(name, layer_type::sigmoid),
            m_input(input) {}

        void SigmoidNode::init(std::string name, std::shared_ptr<Layer> input) {
            m_input = input;
            m_name = name;
            m_type = layer_type::sigmoid;
        }
    }
}
