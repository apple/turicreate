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

- (id _Nonnull)initWithParams:(NSString *_Nonnull)name
                    inputNode:(MPSNNImageNode *_Nonnull)inputNode;

- (MPSNNImageNode *_Nonnull)resultImage;
- (MPSCNNNeuronReLUNode *_Nonnull)underlyingNode;

@end

#endif
