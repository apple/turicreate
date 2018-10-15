#include <unity/toolkits/mps/layer_helpers/output.h>

namespace turi{
    namespace mps {
        OutputNode::OutputNode(std::string name, std::vector<Layer> layers):
            Layer(name, layer_type::output),
            m_layers(layers) {}

        void OutputNode::init(std::string name, std::vector<Layer> layers) {
            m_layers = layers;

            m_name = name;
            m_type = layer_type::output;
        }
    }
}



