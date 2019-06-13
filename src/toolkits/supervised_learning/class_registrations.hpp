#ifndef TURI_SUPERVISED_CLASS_REGISTRATIONS
#define TURI_SUPERVISED_CLASS_REGISTRATIONS

#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/toolkit_class_specification.hpp>

namespace turi {
namespace supervised {

std::vector<turi::toolkit_class_specification> get_toolkit_class_registration();
std::vector<turi::toolkit_function_specification> get_toolkit_function_registration();

}// supervised
}// turi

#endif
