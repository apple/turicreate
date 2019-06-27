#ifndef TURI_NEAREST_NEIGHBORS_CLASS_REGISTRATIONS
#define TURI_NEAREST_NEIGHBORS_CLASS_REGISTRATIONS

#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/toolkit_class_specification.hpp>

namespace turi {
namespace nearest_neighbors {

std::vector<turi::toolkit_class_specification> get_toolkit_class_registration();

}// nearest_neighbors
}// recsys

#endif
