/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#import <ml/neural_net/TCMLComputeUtil.h>

NS_ASSUME_NONNULL_BEGIN

NSData *TCAllocateDataForOutputTensor(MLCTensor *tensor, MLCDeviceType deviceType)
{
  NSData *result = nil;
  if (deviceType == MLCDeviceTypeCPU) {
    NSUInteger size = tensor.descriptor.tensorAllocationSizeInBytes;
    result = [NSData dataWithBytesNoCopy:malloc(size) length:size freeWhenDone:YES];
  } else {
    // On GPU, we need to allocate a page-aligned buffer with size also divisible by page size for
    // GPU devices to write to.
    NSUInteger pageSize = (NSUInteger)(getpagesize());
    NSUInteger numPages = (tensor.descriptor.tensorAllocationSizeInBytes + pageSize - 1) / pageSize;
    NSUInteger size = numPages * pageSize;
    result = [NSData dataWithBytesNoCopy:valloc(size) length:size freeWhenDone:YES];
    // Yes, it would be conceptually cleaner if we returned NSMutableData instead of NSData, since
    // we expect MLCompute to mutate the contents of the memory. But it turns out +[NSMutableData
    // dataWithBytesNoCopy:length:] actually does copy the data (into an internally allocated buffer
    // that is not necessarily page-aligned), despite the initializer name!
  }
  return result;
}

MLCTensorData *TCMLComputeWrapData(NSData *data)
{
  return [MLCTensorData dataWithImmutableBytesNoCopy:data.bytes length:data.length];
}

MLCTensorData *TCMLComputeWrapBuffer(NSMutableData *data)
{
  return [MLCTensorData dataWithBytesNoCopy:data.mutableBytes length:data.length];
}

NS_ASSUME_NONNULL_END
