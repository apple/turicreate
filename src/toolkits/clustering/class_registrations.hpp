#ifndef TURI_CLUSTERING_REGISTRATIONS
#define TURI_CLUSTERING_REGISTRATIONS

#include <core/unity/toolkit_class_macros.hpp>
#include <core/unity/toolkit_class_specification.hpp>

namespace turi {
namespace kmeans {

std::vector<turi::toolkit_class_specification> get_toolkit_class_registration();

}// clustering
}// recsys

#endif

