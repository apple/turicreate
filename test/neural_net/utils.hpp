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

TCMPSResidualDescriptor* define_resiudal_descriptor(boost::property_tree::ptree config);
NSDictionary<NSString *, NSData *>* define_residual_weights(boost::property_tree::ptree weights);

TCMPSDecodingDescriptor*  define_decoding_descriptor(boost::property_tree::ptree config);
NSDictionary<NSString *, NSData *>* define_decoding_weights(boost::property_tree::ptree weights);

TCMPSTransformerDescriptor* define_transformer_descriptor(boost::property_tree::ptree config);
NSDictionary<NSString *, NSData *>* define_transformer_weights(boost::property_tree::ptree weights);

TCMPSVgg16Block1Descriptor* define_block_1_descriptor(boost::property_tree::ptree config);
NSDictionary<NSString *, NSData *>* define_block_1_weights(boost::property_tree::ptree weights);

TCMPSVgg16Block2Descriptor* define_block_2_descriptor(boost::property_tree::ptree config);
NSDictionary<NSString *, NSData *>*  define_block_2_weights(boost::property_tree::ptree weights);

TCMPSVgg16Descriptor* define_vgg_descriptor(boost::property_tree::ptree config);
NSDictionary<NSString *, NSData *>* define_vgg_weights(boost::property_tree::ptree weights);

MPSImageBatch* define_input(boost::property_tree::ptree input, id <MTLDevice> dev);
NSDictionary<NSString *, MPSImageBatch *>* define_loss_input(boost::property_tree::ptree input, id <MTLDevice> dev);
NSData* define_output(boost::property_tree::ptree output);
NSDictionary<NSString *, NSData *>* define_vgg_output(boost::property_tree::ptree output);

bool check_data(NSData* expected, NSData* actual, float epsilon=5e-3);

} // namespace style_transfer
} // namespace neural_net_test

#endif