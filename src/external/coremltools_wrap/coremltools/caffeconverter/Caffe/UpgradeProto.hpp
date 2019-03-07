//
//  UpgradeProto.hpp
//  CoreML
//
//  Created by Srikrishna Sridhar on 2/7/17.
//  Copyright © 2017 Apple Inc. All rights reserved.
//

#ifndef UpgradeProto_hpp
#define UpgradeProto_hpp

#include "caffe_pb_wrapper.hpp"

#include <stdio.h>

namespace CoreMLConverter {

/*
 * Do an inplace update of the caffe protobuf.
 *
 * @param[in] input_filename Path to the input file.
 * @param[in,out] caffeSpec  Caffe model format (possible old format)
 */
void upgradeCaffeNetworkIfNeeded(const std::string input_filename,
                                 caffe::NetParameter& caffeSpec);


}
#endif /* UpgradeProto_hpp */
