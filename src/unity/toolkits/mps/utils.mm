#include <iostream>
#include <vector>
#include <map>

#import <unity/toolkits/mps/utils.h>

namespace turi {
	namespace mps {
		std::shared_ptr<Graph> create_graph(std::vector<std::map<std::string, std::string>>& input) {
			for(auto const& i: input) {
				for (auto const& x : i) {
					std::cout << x.first
							  << ':' 
							  << x.second
							  << std::endl;
				}
			}
			return std::make_shared<Graph>();
		}
	}
}