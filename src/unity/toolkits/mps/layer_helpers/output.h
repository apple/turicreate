#ifndef turi_mps_layer_helpers_output_h
#define turi_mps_layer_helpers_output_h

#include <unity/toolkits/mps/layer_helpers/types.h>
#include <unity/toolkits/mps/layer_helpers/base.h>

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/extensions/model_base.hpp>

#include <string>
#include <vector>

namespace turi{
    namespace mps {
        struct EXPORT OutputNode: public Layer {
            public:
                OutputNode(){};
                OutputNode(std::string name, std::vector<Layer> layers):
                    Layer(name, layer_type::output),
                    m_layers(layers) {};

                std::vector<Layer> m_layers;

                BEGIN_CLASS_MEMBER_REGISTRATION("_OutputNode")
                END_CLASS_MEMBER_REGISTRATION
        };
    }
}

#endif