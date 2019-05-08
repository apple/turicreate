#ifndef fully_connected_h
#define fully_connected_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#import "fully_connected_data_loader.h"

API_AVAILABLE(macos(10.14))
@interface FullyConnected : NSObject {

  int mKernelWidth;
  int mKernelHeight;

  int mInputFeatureChannels;
  int mOutputFeatureChannels;

  float *mWeight;
  float *mBiases;

  NSString *mName;
  MPSCNNFullyConnectedNode *mFullyConnectedNode;
  FullyConnectedDataLoader *mFullyConnectedDataLoad;
  MPSNNImageNode *mInputNode;

  id<MTLDevice> mDevice;
}

- (id _Nonnull)initWithParams:(NSString *_Nonnull)name
         inputFeatureChannels:(int)inputFeatureChannels
        outputFeatureChannels:(int)outputFeatureChannels
                  inputHeight:(int)inputHeight
                   inputWidth:(int)inputWidth
                      weights:(float *_Nonnull)weights
                       biases:(float *_Nonnull)biases
                    inputNode:(MPSNNImageNode *_Nonnull)inputNode
                       device:(id<MTLDevice> _Nonnull)dev
                    cmd_queue:(id<MTLCommandQueue> _Nonnull)cmd_q;

- (MPSNNImageNode *_Nonnull)resultImage;
- (MPSCNNFullyConnectedNode *_Nonnull)underlyingNode;

@end

#endif
