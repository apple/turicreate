#include <unity/toolkits/mps/layer_helpers/addition.h>

namespace turi{
    namespace mps {
        AdditionNode::AdditionNode(std::string name,
                                   std::shared_ptr<Layer> left,
                                   std::shared_ptr<Layer> right):
            Layer(name, layer_type::addition),
            m_left(left),
            m_right(right) {};

        void AdditionNode::init(std::string name,
                                std::shared_ptr<Layer> left,
                                std::shared_ptr<Layer> right) {
            m_left = left;
            m_right = right;

            m_name = name;
            m_type = layer_type::addition;
        }
    }
}
