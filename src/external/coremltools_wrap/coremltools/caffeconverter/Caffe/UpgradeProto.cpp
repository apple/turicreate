//
//  UpgradeProto.cpp
//  CoreML
//
//  Created by Srikrishna Sridhar on 2/7/17.
//  Copyright © 2017 Apple Inc. All rights reserved.
//

#include <iostream>
#include "UpgradeProto.hpp"
#include "caffe/util/upgrade_proto.hpp"


namespace CoreMLConverter {

// Sadly, caffe takes in the input file name only for error messaging
// the input_file doesn't actually get used
void upgradeCaffeNetworkIfNeeded(const std::string input_filename,
                                 caffe::NetParameter& caffeSpec) {

    bool need_upgrade = caffe::NetNeedsUpgrade(caffeSpec);
    bool success = true;
    if (need_upgrade) {
        success = caffe::UpgradeNetAsNeeded(input_filename, &caffeSpec);
        if (!success) {
            std::cout << "Encountered error(s) while upgrading the protobuf; "
                      << "see details above." << std::endl;
        }
    }
}


} // namespace CoreMLConverter
