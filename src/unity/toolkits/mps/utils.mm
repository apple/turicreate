#include <iostream>
#include <vector>
#include <map>

#import <Foundation/Foundation.h>

#import <unity/toolkits/mps/utils.h>
#import <unity/toolkits/mps/layer_helpers/types.h>

namespace turi {
    namespace mps {

        std::shared_ptr<Graph> create_graph(std::map<std::string, std::shared_ptr<Layer>>& layer_dict, std::vector<std::shared_ptr<Layer>> layer_arr) {
            /* TODO: MOVE THIS INTO COMPILE IN THE GRAPH OBJECT */

            std::shared_ptr<Graph> grph = std::make_shared<Graph>();

            // TODO: populate graph
            return grph;
        }

        int test() {
          return 3;
        }
    }
}