#ifndef addition_layer_h
#define addition_layer_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

API_AVAILABLE(macos(10.14))
@interface AdditionLayer : NSObject {
  NSString *mName;
  MPSNNAdditionNode *mAdditionNode;
}

- (id)initWithParams:(NSString *)name
            leftNode:(MPSNNImageNode *)leftNode
           rightNode:(MPSNNImageNode *)rightNode;

- (MPSNNImageNode *_Nonnull)resultImage;

- (MPSNNAdditionNode *)underlyingNode;

@end

#endif
