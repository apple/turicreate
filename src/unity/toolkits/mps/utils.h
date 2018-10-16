#ifndef turi_mps_utils_h
#define turi_mps_utils_h

#include <unity/toolkits/mps/graph.h>
#include <unity/toolkits/mps/layer_helpers/base.h>

#include <string>
#include <vector>

namespace turi {
    namespace mps { 
        std::shared_ptr<Graph> create_graph(std::map<std::string, std::shared_ptr<Layer>>& layer_dict, std::vector<std::shared_ptr<Layer>> layer_arr);
        
        int test();
    }
}

#endif