#ifndef TURI_ANNOTATIONS_CLASS_REGISTRATIONS_HPP
#define TURI_ANNOTATIONS_CLASS_REGISTRATIONS_HPP

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_class_specification.hpp>

namespace turi {
namespace annotate {

std::vector<turi::toolkit_class_specification> get_toolkit_class_registration();
std::vector<turi::toolkit_function_specification>
get_toolkit_function_registration();

} // namespace annotate
} // namespace turi

#endif