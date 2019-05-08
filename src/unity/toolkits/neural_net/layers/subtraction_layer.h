#ifndef subtraction_layer_h
#define subtraction_layer_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

API_AVAILABLE(macos(10.14))
@interface SubtractionLayer : NSObject {
  NSString *mName;
  MPSNNSubtractionNode *mSubtractionNode;
}

- (id)initWithParams:(NSString *)name
            leftNode:(MPSNNImageNode *)leftNode
           rightNode:(MPSNNImageNode *)rightNode;

- (MPSNNImageNode *_Nonnull)resultImage;

- (MPSNNSubtractionNode *)underlyingNode;

@end

#endif
