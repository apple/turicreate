#ifndef turi_mps_nearest_upsampling_layer_h
#define turi_mps_nearest_upsampling_layer_h

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h> 

API_AVAILABLE(macos(10.13))
@interface NearestUpsamplingLayer: NSObject {
    NSString *mName;
    MPSCNNUpsamplingNearestNode *mUpsamplingNode;
}

- (id __nonnull) initWithParams:(NSString *__nonnull)name
                         scaleX:(int)scaleX
                         scaleY:(int)scaleY
                      inputNode:(MPSNNImageNode *__nonnull)inputNode;

- (MPSNNImageNode *__nonnull) resultImage;

@end
    

#endif
