#ifndef TURI_SUPERVISED_CLASS_REGISTRATIONS
#define TURI_SUPERVISED_CLASS_REGISTRATIONS

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_class_specification.hpp>

namespace turi {
namespace supervised {

std::vector<turi::toolkit_class_specification> get_toolkit_class_registration();
std::vector<turi::toolkit_function_specification> get_toolkit_function_registration();

}// supervised
}// turi 

#endif

