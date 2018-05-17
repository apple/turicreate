#ifndef TURI_CLUSTERING_REGISTRATIONS
#define TURI_CLUSTERING_REGISTRATIONS

#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_class_specification.hpp>

namespace turi {
namespace kmeans {

std::vector<turi::toolkit_class_specification> get_toolkit_class_registration();

}// clustering
}// recsys

#endif

