#ifndef turi_mps_utils_h
#define turi_mps_utils_h

#include <unity/toolkits/mps/graph.h>

namespace turi {
	namespace mps {	
		std::shared_ptr<Graph> create_graph(std::vector<std::map<std::string, std::string>>& input);
	}
}

#endif