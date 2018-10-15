#ifndef turi_mps_relu_layer_h
#define turi_mps_relu_layer_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>	

API_AVAILABLE(macos(10.13))
@interface ReluLayer: NSObject {
	NSString *mName;
	MPSCNNNeuronReLUNode *mReluNode;
}

- (id __nonnull) initWithParams:(NSString *__nonnull)name
					  inputNode:(MPSNNImageNode *__nonnull)inputNode;

- (MPSNNImageNode *__nonnull) resultImage;

@end
	
#endif
