#include <toolkits/nearest_neighbors/ball_tree_neighbors.hpp>
#include <toolkits/nearest_neighbors/brute_force_neighbors.hpp>
#include <toolkits/nearest_neighbors/lsh_neighbors.hpp>

namespace turi {
namespace nearest_neighbors {


BEGIN_CLASS_REGISTRATION
REGISTER_CLASS(ball_tree_neighbors)
REGISTER_CLASS(brute_force_neighbors)
REGISTER_CLASS(lsh_neighbors)
END_CLASS_REGISTRATION

}}
