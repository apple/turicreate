#ifndef turi_mps_layer_helpers_sigmoid_h
#define turi_mps_layer_helpers_sigmoid_h

#include <unity/toolkits/mps/layer_helpers/types.h>
#include <unity/toolkits/mps/layer_helpers/base.h>

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/extensions/model_base.hpp>

#include <string>
#include <vector>

namespace turi{
    namespace mps {
        struct EXPORT SigmoidNode: public Layer {
            public:
                SigmoidNode(){};
                SigmoidNode(std::string name, std::shared_ptr<Layer> input):
                    Layer(name, layer_type::sigmoid),
                    m_input(input) {};

                std::shared_ptr<Layer> m_input;

                BEGIN_CLASS_MEMBER_REGISTRATION("SigmoidNode")
                END_CLASS_MEMBER_REGISTRATION
        };
    }
}

#endif