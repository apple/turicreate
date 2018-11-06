#ifndef turi_mps_input_layer_h
#define turi_mps_input_layer_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

API_AVAILABLE(macos(10.13))
@interface InputLayer : NSObject
{
	MPSNNImageNode * mResult;
    id<MTLDevice> mDevice;
}

- (id __nonnull) initInput;

- (MPSNNImageNode *__nonnull) resultImage;

@end

#endif
