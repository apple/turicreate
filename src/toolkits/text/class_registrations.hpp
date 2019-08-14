#ifndef TURI_TEXT_CLASS_REGISTRATIONS
#define TURI_TEXT_CLASS_REGISTRATIONS

#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/toolkit_class_specification.hpp>

namespace turi {
namespace text {

std::vector<turi::toolkit_class_specification> get_toolkit_class_registration();

}// text
}// recsys

#endif
