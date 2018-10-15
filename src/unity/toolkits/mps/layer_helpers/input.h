#ifndef turi_mps_layer_helpers_input_h
#define turi_mps_layer_helpers_input_h

#include <unity/toolkits/mps/layer_helpers/types.h>
#include <unity/toolkits/mps/layer_helpers/base.h>

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/extensions/model_base.hpp>

#include <string>
#include <vector>

namespace turi{
    namespace mps {
        struct EXPORT InputNode: public Layer {
            public:
                InputNode(){};
                InputNode(std::string name): Layer(name, layer_type::input) {};

                BEGIN_CLASS_MEMBER_REGISTRATION("InputNode")
                END_CLASS_MEMBER_REGISTRATION
        };
    }
}

#endif