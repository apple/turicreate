#ifndef turi_mps_layer_helpers_pooling_h
#define turi_mps_layer_helpers_pooling_h

#include <unity/toolkits/mps/layer_helpers/types.h>
#include <unity/toolkits/mps/layer_helpers/base.h>

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/extensions/model_base.hpp>

#include <string>
#include <vector>

namespace turi{
    namespace mps {
        struct EXPORT PoolingNode: public Layer {
            public:
                PoolingNode(){};
                PoolingNode(std::string name,
                            std::shared_ptr<Layer> input,
                            size_t kernel_width,
                            size_t kernel_height,
                            size_t stride_in_pixels_x,
                            size_t stride_in_pixels_y,
                            size_t type);

                void init(std::string name,
                          std::shared_ptr<Layer> input,
                          std::pair<size_t, size_t> kernel,
                          std::pair<size_t, size_t> stride,
                          size_t type);

                std::shared_ptr<Layer> m_input;
                size_t m_kernel_width;
                size_t m_kernel_height;
                size_t m_stride_in_pixels_x;
                size_t m_stride_in_pixels_y;
                pooling_type m_pooling;


                BEGIN_CLASS_MEMBER_REGISTRATION("_PoolingNode")
                REGISTER_CLASS_MEMBER_FUNCTION(PoolingNode::init, "name", "input", "kernel", "stride", "type")
                END_CLASS_MEMBER_REGISTRATION
        };
    }
}

#endif