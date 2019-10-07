/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef __TEST_NEURAL_NET_STYLE_TRANSFER_TESTING_HPP
#define __TEST_NEURAL_NET_STYLE_TRANSFER_TESTING_HPP

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace neural_net_test {
namespace style_transfer {

class BaseNetworkTest {
 public:
  BaseNetworkTest() = delete;
  explicit BaseNetworkTest(float epsilon);

  ~BaseNetworkTest();

  /**
    Checks the encoding prediction by using an input configuration and an input
    bin file. The configuration file has 3 major keys:

      - height
      - width
      - channels

    The output from the TCMPS inference is then checked against the output
    bin file:

      - output

    If there is an element-wise diff greater than an epsilon value of 5e-3 the
    method returns false.

    @param input  - an `std::string` containing the path to the input directory
    @param output - an `std::string` containing the path to the output directory

    @return - a boolean of whether the element-wise diff between the TCMPS
              inference and the expected inference exceeds an epsilon of 5e-3.
              If this epsilon is exceeded, the method returns `false`. If the
              element diff is within this epsilon the method returns `true`.
  */

  virtual bool check_predict(std::string input, std::string output);

 protected:
  struct common_impl;
  std::unique_ptr<common_impl> internal_impl;
  float m_epsilon;
};

};  // namespace style_transfer
};  // namespace neural_net_test

#endif