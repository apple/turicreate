/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TEST_NEURAL_NET_STYLE_TRANSFER_UTILS
#define TEST_NEURAL_NET_STYLE_TRANSFER_UTILS

#include <functional>
#include <map>
#include <memory>

#include <boost/property_tree/ptree.hpp>

namespace neural_net_test {
namespace style_transfer {

struct EncodingTest {
public: 
  EncodingTest(boost::property_tree::ptree config,
               boost::property_tree::ptree weights);
  ~EncodingTest();
  /**
    Checks the encoding prediction by using an input dictionary with four keys
    present:
    
      - content
      - height
      - width
      - channels
    
    The output from the TCMPS inference is then checked against the output
    dictionary with one key:

      - output

    If there is an element-wise diff greater than an epsilon value of 5e-3 the
    method returns false.

    @param input  - a `boost::property_tree::ptree` containing four keys
    @param output - a `boost::property_tree::ptree` containing the expected
                    output

    @return - a boolean of whether the element-wise diff between the TCMPS
              inference and the expected inference exceeds an epsilon of 5e-3.
              If this epsilon is exceeded, the method returns `false`. If the
              element diff is within this epsilon the method returns `true`.
  */
  bool check_predict(boost::property_tree::ptree input,
                     boost::property_tree::ptree output);
private:
  struct impl;
  std::unique_ptr<impl> m_impl;
};

struct ResidualTest {
public:
  ResidualTest(boost::property_tree::ptree config,
               boost::property_tree::ptree weights);
  ~ResidualTest();

  /**
    Checks the residual prediction by using an input dictionary with four keys
    present:
    
      - content
      - height
      - width
      - channels
    
    The output from the TCMPS inference is then checked against the output
    dictionary with one key:

      - output

    If there is an element-wise diff greater than an epsilon value of 5e-3 the
    method returns false.

    @param input  - a `boost::property_tree::ptree` containing four keys
    @param output - a `boost::property_tree::ptree` containing the expected
                    output

    @return - a boolean of whether the element-wise diff between the TCMPS
              inference and the expected inference exceeds an epsilon of 5e-3.
              If this epsilon is exceeded, the method returns `false`. If the
              element diff is within this epsilon the method returns `true`.
  */
  bool check_predict(boost::property_tree::ptree input,
                     boost::property_tree::ptree output);
private:
  struct impl;
  std::unique_ptr<impl> m_impl;
};

struct DecodingTest {
public: 
  DecodingTest(boost::property_tree::ptree config,
               boost::property_tree::ptree weights);
  ~DecodingTest();

  /**
    Checks the decoding prediction by using an input dictionary with four keys
    present:
    
      - content
      - height
      - width
      - channels
    
    The output from the TCMPS inference is then checked against the output
    dictionary with one key:

      - output

    If there is an element-wise diff greater than an epsilon value of 5e-3 the
    method returns false.

    @param input  - a `boost::property_tree::ptree` containing four keys
    @param output - a `boost::property_tree::ptree` containing the expected
                    output

    @return - a boolean of whether the element-wise diff between the TCMPS
              inference and the expected inference exceeds an epsilon of 5e-3.
              If this epsilon is exceeded, the method returns `false`. If the
              element diff is within this epsilon the method returns `true`.
  */
  bool check_predict(boost::property_tree::ptree input,
                     boost::property_tree::ptree output);
private:
  struct impl;
  std::unique_ptr<impl> m_impl;
};

struct ResnetTest {
public: 
  ResnetTest(boost::property_tree::ptree config,
             boost::property_tree::ptree weights);
  ~ResnetTest();

  /**
    Checks the resnet prediction by using an input dictionary with four keys
    present:
    
      - content
      - height
      - width
      - channels
    
    The output from the TCMPS inference is then checked against the output
    dictionary with one key:

      - output

    If there is an element-wise diff greater than an epsilon value of 5e-3 the
    method returns false.

    @param input  - a `boost::property_tree::ptree` containing four keys
    @param output - a `boost::property_tree::ptree` containing the expected
                    output

    @return - a boolean of whether the element-wise diff between the TCMPS
              inference and the expected inference exceeds an epsilon of 5e-2.
              If this epsilon is exceeded, the method returns `false`. If the
              element diff is within this epsilon the method returns `true`.
  */
  bool check_predict(boost::property_tree::ptree input,
                     boost::property_tree::ptree output);
private:
  struct impl;
  std::unique_ptr<impl> m_impl; 
};

struct Block1Test {
public: 
  Block1Test(boost::property_tree::ptree config,
             boost::property_tree::ptree weights);
  ~Block1Test();

  /**
    Checks the VGG Block 1 prediction by using an input dictionary with four
    keys present:
    
      - content
      - height
      - width
      - channels
    
    The output from the TCMPS inference is then checked against the output
    dictionary with one key:

      - output

    If there is an element-wise diff greater than an epsilon value of 5e-3 the
    method returns false.

    @param input  - a `boost::property_tree::ptree` containing four keys
    @param output - a `boost::property_tree::ptree` containing the expected
                    output

    @return - a boolean of whether the element-wise diff between the TCMPS
              inference and the expected inference exceeds an epsilon of 5e-3.
              If this epsilon is exceeded, the method returns `false`. If the
              element diff is within this epsilon the method returns `true`.
  */
  bool check_predict(boost::property_tree::ptree input,
                     boost::property_tree::ptree output);
private:
  struct impl;
  std::unique_ptr<impl> m_impl; 
};

struct Block2Test {
public: 
  Block2Test(boost::property_tree::ptree config,
             boost::property_tree::ptree weights);
  ~Block2Test();

  /**
    Checks the VGG Block 2 prediction by using an input dictionary with four
    keys present:
    
      - content
      - height
      - width
      - channels
    
    The output from the TCMPS inference is then checked against the output
    dictionary with one key:

      - output

    If there is an element-wise diff greater than an epsilon value of 5e-3 the
    method returns false.

    @param input  - a `boost::property_tree::ptree` containing four keys
    @param output - a `boost::property_tree::ptree` containing the expected
                    output

    @return - a boolean of whether the element-wise diff between the TCMPS
              inference and the expected inference exceeds an epsilon of 5e-3.
              If this epsilon is exceeded, the method returns `false`. If the
              element diff is within this epsilon the method returns `true`.
  */
  bool check_predict(boost::property_tree::ptree input,
                     boost::property_tree::ptree output);
private:
  struct impl;
  std::unique_ptr<impl> m_impl; 
};

struct Vgg16Test {
public: 
  Vgg16Test(boost::property_tree::ptree config,
            boost::property_tree::ptree weights);
  ~Vgg16Test();
  /**
    Checks the VGG Networks prediction by using an input dictionary with four
    keys present:
    
      - content
      - height
      - width
      - channels
    
    The output from the TCMPS inference is then checked against the output
    dictionary with one key:

      - output

    If there is an element-wise diff greater than an epsilon value of 5e-3 the
    method returns false.

    @param input  - a `boost::property_tree::ptree` containing four keys
    @param output - a `boost::property_tree::ptree` containing the expected
                    output

    @return - a boolean of whether the element-wise diff between the TCMPS
              inference and the expected inference exceeds an epsilon of 5e-3.
              If this epsilon is exceeded, the method returns `false`. If the
              element diff is within this epsilon the method returns `true`.
  */
  bool check_predict(boost::property_tree::ptree input,
                     boost::property_tree::ptree output);
private:
  struct impl;
  std::unique_ptr<impl> m_impl; 
};

struct LossTest {
public: 
  LossTest(boost::property_tree::ptree resnet_config,
           boost::property_tree::ptree vgg_config,
           boost::property_tree::ptree weights);
  
  ~LossTest();

  bool check_predict(boost::property_tree::ptree input,
                     boost::property_tree::ptree output);
private:
  struct impl;
  std::unique_ptr<impl> m_impl; 
};

struct WeightUpdateTest {
public: 
  WeightUpdateTest(boost::property_tree::ptree config);
  ~WeightUpdateTest();
private:
  struct impl;
  std::unique_ptr<impl> m_impl; 
};

} // namespace style_transfer
} // namespace neural_net_test

#endif