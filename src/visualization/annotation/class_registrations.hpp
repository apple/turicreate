#ifndef TURI_ANNOTATIONS_CLASS_REGISTRATIONS_HPP
#define TURI_ANNOTATIONS_CLASS_REGISTRATIONS_HPP

#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/toolkit_class_specification.hpp>

namespace turi {
namespace annotate {

std::vector<turi::toolkit_class_specification> get_toolkit_class_registration();
std::vector<turi::toolkit_function_specification>
get_toolkit_function_registration();

} // namespace annotate
} // namespace turi

#endif
