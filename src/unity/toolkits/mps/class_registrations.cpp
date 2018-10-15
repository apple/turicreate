#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/toolkits/mps/class_registrations.hpp>

#include <unity/toolkits/mps/graph.h>

#include <unity/toolkits/mps/layer_helpers/addition.h>
#include <unity/toolkits/mps/layer_helpers/base.h>
#include <unity/toolkits/mps/layer_helpers/convolution.h>
#include <unity/toolkits/mps/layer_helpers/input.h>
#include <unity/toolkits/mps/layer_helpers/instance_norm.h>
#include <unity/toolkits/mps/layer_helpers/output.h>
#include <unity/toolkits/mps/layer_helpers/pooling.h>
#include <unity/toolkits/mps/layer_helpers/relu.h>
#include <unity/toolkits/mps/layer_helpers/sigmoid.h>
#include <unity/toolkits/mps/layer_helpers/upsampling.h>


#include <unity/toolkits/mps/utils.h>

using namespace turi;

namespace turi {
    namespace mps {
        BEGIN_FUNCTION_REGISTRATION
        REGISTER_FUNCTION(create_graph, "input");
        END_FUNCTION_REGISTRATION
    }
}

BEGIN_CLASS_REGISTRATION
REGISTER_CLASS(turi::mps::Graph)
REGISTER_CLASS(turi::mps::Layer)
REGISTER_CLASS(turi::mps::InputNode)
REGISTER_CLASS(turi::mps::OutputNode)
REGISTER_CLASS(turi::mps::AdditionNode)
REGISTER_CLASS(turi::mps::ConvolutionNode)
REGISTER_CLASS(turi::mps::InstanceNormNode)
REGISTER_CLASS(turi::mps::ReluNode)
REGISTER_CLASS(turi::mps::SigmoidNode)
END_CLASS_REGISTRATION