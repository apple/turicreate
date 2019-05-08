#ifndef instance_norm_layer_h
#define instance_norm_layer_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#import "instance_norm_data_loader.h"

API_AVAILABLE(macos(10.14))
@interface InstanceNormLayer : NSObject {
  NSString *mName;

  int mChannels;
  int mStyles;

  float **mGamma;
  float **mBeta;

  MPSNNImageNode *mInputNode;
  MPSCNNInstanceNormalizationNode *mInstNormNode;
  InstanceNormDataLoader *mInstNormDataLoad;

  id<MTLDevice> mDevice;
}

- (id)initWithParameters:(NSString *)name
                channels:(int)channels
                  styles:(int)styles
                   gamma:(float **)gamma
                    beta:(float **)beta
               inputNode:(MPSNNImageNode *)inputNode
                  device:(id<MTLDevice> _Nonnull)dev
               cmd_queue:(id<MTLCommandQueue> _Nonnull)cmd_q;

- (MPSCNNInstanceNormalizationNode *)underlyingNode;
- (InstanceNormDataLoader *)underlyingDataLoader;
- (MPSNNImageNode *_Nonnull)resultImage;

@end

#endif
