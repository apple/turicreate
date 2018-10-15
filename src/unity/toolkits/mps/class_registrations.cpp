#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/toolkits/mps/class_registrations.hpp>

#include <unity/toolkits/mps/graph.h>

#include <unity/toolkits/mps/layer_helpers/addition.h>
#include <unity/toolkits/mps/layer_helpers/base.h>
#include <unity/toolkits/mps/layer_helpers/convolution.h>
#include <unity/toolkits/mps/layer_helpers/input.h>
#include <unity/toolkits/mps/layer_helpers/output.h>

#include <unity/toolkits/mps/utils.h>


namespace turi {
	namespace mps {
		BEGIN_FUNCTION_REGISTRATION
		REGISTER_FUNCTION(create_graph, "input");
		END_FUNCTION_REGISTRATION

		BEGIN_CLASS_REGISTRATION
		REGISTER_CLASS(Graph)
		REGISTER_CLASS(Layer)
		REGISTER_CLASS(InputNode)
		REGISTER_CLASS(OutputNode)
		REGISTER_CLASS(AdditionNode)
		REGISTER_CLASS(ConvolutionNode)
		END_CLASS_REGISTRATION
	}
}