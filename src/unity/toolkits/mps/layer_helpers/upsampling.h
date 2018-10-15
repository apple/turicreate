#ifndef turi_mps_layer_helpers_upsampling_h
#define turi_mps_layer_helpers_upsampling_h

#include <unity/toolkits/mps/layer_helpers/types.h>
#include <unity/toolkits/mps/layer_helpers/base.h>

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/extensions/model_base.hpp>

#include <string>
#include <vector>

namespace turi{
    namespace mps {
        struct EXPORT UpsamplingNode: public Layer {
            public:
                UpsamplingNode(){};
                UpsamplingNode(std::string name,
                               std::shared_ptr<Layer> input,
                               size_t scale_x,
                               size_t scale_y,
                               size_t type):
                    Layer(name, layer_type::upsampling),
                    m_input(input),
                    m_scale_x(scale_x),
                    m_scale_y(scale_y),
                    m_type(static_cast<upsampling_type>(type)) {};

                std::shared_ptr<Layer> m_input;
                size_t m_scale_x;
                size_t m_scale_y;
                upsampling_type m_type;

                BEGIN_CLASS_MEMBER_REGISTRATION("UpsamplingNode")
                END_CLASS_MEMBER_REGISTRATION
        };
    }
}

#endif