#ifndef multiplication_layer_h
#define multiplication_layer_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

API_AVAILABLE(macos(10.14))
@interface MultiplicationLayer : NSObject {
  NSString *mName;
  MPSNNMultiplicationNode *mMultiplicationNode;
}

- (id)initWithParams:(NSString *)name
            leftNode:(MPSNNImageNode *)leftNode
           rightNode:(MPSNNImageNode *)rightNode;

- (MPSNNImageNode *_Nonnull)resultImage;

- (MPSNNMultiplicationNode *)underlyingNode;

@end

#endif
