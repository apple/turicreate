#ifndef turi_mps_addition_layer_h
#define turi_mps_addition_layer_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>	

API_AVAILABLE(macos(10.13))
@interface AdditionLayer: NSObject {
	NSString *mName;
	MPSNNAdditionNode *mAdditionNode;
}

- (id __nonnull) initWithParams:(NSString * __nonnull)name
					   leftNode:(MPSNNImageNode * __nonnull)leftNode
					  rightNode:(MPSNNImageNode * __nonnull)rightNode;

- (MPSNNImageNode * __nonnull) resultImage;

@end
	
#endif
