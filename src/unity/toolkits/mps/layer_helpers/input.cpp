#include <unity/toolkits/mps/layer_helpers/input.h>

namespace turi{
    namespace mps {
        InputNode::InputNode(std::string name):
            Layer(name, layer_type::input) {};

        void InputNode::init(std::string name) {
            m_name = name;
            m_type = layer_type::input;
        }
    }
}