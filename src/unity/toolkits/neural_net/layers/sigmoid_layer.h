#ifndef sigmoid_layer_h
#define sigmoid_layer_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

API_AVAILABLE(macos(10.14))
@interface SigmoidLayer : NSObject {
  NSString *mName;
  MPSCNNNeuronSigmoidNode *mSigmoidNode;
}

- (id)initWithParams:(NSString *)name inputNode:(MPSNNImageNode *)inputNode;

- (MPSCNNNeuronSigmoidNode *)underlyingNode;
- (MPSNNImageNode *_Nonnull)resultImage;

@end

#endif
