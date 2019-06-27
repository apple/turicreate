/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <string>
#include <core/random/random.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/sframe_index_mapping.hpp>
#include <toolkits/recsys/models.hpp>
#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>
#include <timer/timer.hpp>

using namespace turi;
using namespace turi::recsys;
using namespace turi::v2;

int main(int argc, char **argv) {

  for(size_t i = 0; i < 10000; ++i) {

    timer tt;

    tt.start();

    for(size_t i = 0; i < 100; ++i) {

      make_testing_sarray(flex_type_enum::INTEGER, {1}); 
    }
    
    std::cerr << "Recommend time / 1000: " << tt.current_time_millis() << "ms." << std::endl;
  }

  return 0; 
}
