#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/toolkits/mps/class_registrations.hpp>
#include <unity/toolkits/mps/generator.hpp>

namespace turi {
	namespace mps {
		BEGIN_FUNCTION_REGISTRATION
		REGISTER_FUNCTION(create_graph, "input");
		END_FUNCTION_REGISTRATION

		BEGIN_CLASS_REGISTRATION
		REGISTER_CLASS(Graph)
		END_CLASS_REGISTRATION
	}
}