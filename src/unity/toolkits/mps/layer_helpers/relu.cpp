#include <unity/toolkits/mps/layer_helpers/relu.h>

namespace turi{
    namespace mps {
        ReluNode::ReluNode(std::string name, std::shared_ptr<Layer> input):
            Layer(name, layer_type::relu),
            m_input(input) {}

        void ReluNode::init(std::string name, std::shared_ptr<Layer> input) {
            m_input = input;
            m_name = name;
            m_type = layer_type::relu;
        }
    }
}
