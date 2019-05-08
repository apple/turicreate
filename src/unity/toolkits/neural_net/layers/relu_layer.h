#ifndef relu_layer_h
#define relu_layer_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

API_AVAILABLE(macos(10.14))
@interface ReluLayer : NSObject {
  NSString *mName;
  MPSCNNNeuronReLUNode *mReluNode;
}

- (id)initWithParams:(NSString *)name inputNode:(MPSNNImageNode *)inputNode;

- (MPSNNImageNode *_Nonnull)resultImage;
- (MPSCNNNeuronReLUNode *)underlyingNode;

@end

#endif
