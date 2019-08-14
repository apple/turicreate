#ifndef TURI_RECSYS_CLASS_REGISTRATIONS
#define TURI_RECSYS_CLASS_REGISTRATIONS

#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/toolkit_class_specification.hpp>

namespace turi {
namespace recsys {

std::vector<turi::toolkit_class_specification> get_toolkit_class_registration();

}// feature_engineering
}// recsys

#endif
