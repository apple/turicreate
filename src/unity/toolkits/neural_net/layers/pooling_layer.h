#ifndef pooling_layer_h
#define pooling_layer_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

API_AVAILABLE(macos(10.14))
@interface PoolingLayer : NSObject {
  NSString *mName;
  MPSCNNPoolingAverageNode *mPoolingNode;
}

- (id _Nonnull)initWithParams:(NSString *_Nonnull)name
                    inputNode:(MPSNNImageNode *_Nonnull)inputNode
                  kernelWidth:(int)kernelWidth
                 kernelHeight:(int)kernelHeight
                  strideWidth:(int)strideWidth
                 strideHeight:(int)strideHeight;

- (MPSNNImageNode *_Nonnull)resultImage;

- (MPSCNNPoolingAverageNode *_Nonnull)underlyingNode;

@end

#endif
