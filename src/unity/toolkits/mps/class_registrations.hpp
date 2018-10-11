#ifndef TURI_TOOLKIT_MPS_REGISTRATIONS_H_
#define TURI_TOOLKIT_MPS_REGISTRATIONS_H_


#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/toolkit_function_specification.hpp>

namespace turi {
	namespace mps {
		std::vector<turi::toolkit_function_specification> get_toolkit_function_registration();
	}
}
#endif
