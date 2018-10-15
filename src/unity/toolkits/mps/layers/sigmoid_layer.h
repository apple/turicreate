#ifndef turi_mps_sigmoid_layer_h
#define turi_mps_sigmoid_layer_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>	

API_AVAILABLE(macos(10.13))
@interface SigmoidLayer: NSObject {
	NSString *mName;
	MPSCNNNeuronSigmoidNode *mSigmoidNode;
}

- (id __nonnull) initWithParams:(NSString *__nonnull)name
					  inputNode:(MPSNNImageNode *__nonnull)inputNode;

- (MPSNNImageNode *__nonnull) resultImage;

@end
	
#endif
