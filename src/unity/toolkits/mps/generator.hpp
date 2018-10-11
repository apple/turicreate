#ifndef __TURI_MPS_GENERATOR_H
#define __TURI_MPS_GENERATOR_H

#include <unity/toolkits/mps/graph.hpp>

namespace turi {
	namespace mps {	
		std::shared_ptr<Graph> create_graph(std::vector<std::map<std::string, std::string>>& input);
	}
}

#endif