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
                InputNode(std::string name);

                void init(std::string name);

                BEGIN_CLASS_MEMBER_REGISTRATION("_InputNode")
                REGISTER_CLASS_MEMBER_FUNCTION(InputNode::init, "name", "input")
                END_CLASS_MEMBER_REGISTRATION
        };
    }
}

#endif