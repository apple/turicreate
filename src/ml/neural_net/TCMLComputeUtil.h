/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#import <Foundation/Foundation.h>
#import <MLCompute/MLCompute.h>

NS_ASSUME_NONNULL_BEGIN

#ifdef __cplusplus
extern "C" {
#endif

typedef NS_ENUM(int32_t, TCMLComputeTensorSize) {
  TCMLComputeTensorSizeBatch,
  TCMLComputeTensorSizeChannels,
  TCMLComputeTensorSizeHeight,
  TCMLComputeTensorSizeWidth,
};

/// Returns a buffer suitable to pass to MLCompute as memory into which to write an output tensor's
/// value. MLCompute requires this memory to be page-aligned in the GPU case.
API_AVAILABLE(macos(10.16))
NSData *TCAllocateDataForOutputTensor(MLCTensor *tensor, MLCDeviceType deviceType);

API_AVAILABLE(macos(10.16))
MLCTensorData *TCMLComputeWrapData(NSData *data);

API_AVAILABLE(macos(10.16))
MLCTensorData *TCMLComputeWrapBuffer(NSMutableData *data);

#ifdef __cplusplus
}  // extern "C"
#endif

NS_ASSUME_NONNULL_END
