/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TEST_NEURAL_NET_UTILS
#define TEST_NEURAL_NET_UTILS

#include <boost/property_tree/ptree.hpp>

#import <ml/neural_net/style_transfer/mps_style_transfer_decoding_node.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_encoding_node.h>
#import <ml/neural_net/style_transfer/mps_style_transfer_residual_node.h>

namespace neural_net_test {
namespace style_transfer {

TCMPSEncodingDescriptor* define_encoding_descriptor(boost::property_tree::ptree config);
NSDictionary<NSString *, NSData *>* define_encoding_weights(boost::property_tree::ptree weights);

MPSImageBatch* define_input(boost::property_tree::ptree input, id <MTLDevice> dev);
NSData* define_output(boost::property_tree::ptree output);
bool check_data(NSData* expected, NSData* actual, float epsilon=5e-3);

} // namespace style_transfer
} // namespace neural_net_test

#endif