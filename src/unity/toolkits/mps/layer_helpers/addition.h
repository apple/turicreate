#ifndef turi_mps_layer_helpers_addition_h
#define turi_mps_layer_helpers_addition_h

#include <unity/toolkits/mps/layer_helpers/types.h>
#include <unity/toolkits/mps/layer_helpers/base.h>

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/extensions/model_base.hpp>

#include <string>
#include <vector>

namespace turi{
    namespace mps {
        struct EXPORT AdditionNode: public Layer {
            std::shared_ptr<Layer> m_left;
            std::shared_ptr<Layer> m_right;

            AdditionNode(){};

            AdditionNode(std::string name, std::shared_ptr<Layer> left, std::shared_ptr<Layer> right):
                Layer(name, layer_type::addition),
                m_left(left),
                m_right(right) {};

            BEGIN_CLASS_MEMBER_REGISTRATION("AdditionNode")
            REGISTER_CLASS_DOCSTRING("A simple AdditionNode.")
            END_CLASS_MEMBER_REGISTRATION
        };
    }
}

#endif