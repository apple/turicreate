#ifndef softmax_layer_h
#define softmax_layer_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

API_AVAILABLE(macos(10.14))
@interface SoftmaxLayer : NSObject {
  NSString *mName;
  MPSCNNSoftMaxNode *mSoftmaxNode;
}

- (id)initWithParams:(NSString *)name inputNode:(MPSNNImageNode *)inputNode;

- (MPSCNNSoftMaxNode *)underlyingNode;
- (MPSNNImageNode *_Nonnull)resultImage;

@end

#endif