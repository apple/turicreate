//
//  mps_lstm_helper.h
//  tcmps
//
//  Created by ilai giloh on 29/04/2018.
//  Copyright © 2018 Turi. All rights reserved.
//

#ifndef mps_lstm_helper_h
#define mps_lstm_helper_h

#import <MetalPerformanceShaders/MetalPerformanceShaders.h>
#import <string>
#import <map>


namespace turi {
namespace neural_net {

extern const std::string lstm_weight_names_mxnet_format[12];

API_AVAILABLE(macos(10.13))
MPSMatrix * createWeightMatrix(id <MTLDevice> device, MPSRNNMatrixId wMatId, int inputFeatures, int outputFeatures);
MPSRNNMatrixId MxnetNameToMatrixId (std::string mat_name);
API_AVAILABLE(macos(10.13))
MPSVector * MPSMatrixToVector (MPSMatrix * matrix);
API_AVAILABLE(macos(10.13))
void printMatrix(MPSMatrix * matrix, const char* name, NSUInteger byteOffset);

}  // namespace neural_net
}  // namespace turi

#endif /* mps_lstm_helper_h */
