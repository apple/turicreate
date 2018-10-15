#include <unity/toolkits/mps/layer_helpers/instance_norm.h>

namespace turi{
    namespace mps {
        InstanceNormNode::InstanceNormNode(std::string name, 
                                 std::shared_ptr<Layer> input,
                                 size_t channels,
                                 size_t styles,
                                 std::vector<std::vector<float>> gamma,
                                 std::vector<std::vector<float>> beta):
            Layer(name, layer_type::instance_norm),
            m_input(input),
            m_channels(channels),
            m_styles(styles),
            m_gamma(gamma),
            m_beta(beta) {}

        void InstanceNormNode::init(std::string name, 
                                    std::shared_ptr<Layer> input,
                                    size_t channels,
                                    size_t styles,
                                    std::map<std::string, std::vector<std::vector<float>>> data) {
            m_input = input;

            m_channels = channels;
            m_styles = styles;

            m_gamma = data.find("gamma")->second;
            m_beta = data.find("beta")->second;

            m_name = name;
            m_type = layer_type::output;
        }
    }
}



