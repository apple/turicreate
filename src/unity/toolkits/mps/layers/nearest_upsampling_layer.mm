#import <unity/toolkits/mps/layers/nearest_upsampling_layer.h>

@implementation NearestUpsamplingLayer 

- (id __nonnull) initWithParams:(NSString *__nonnull)name
						 scaleX:(int)scaleX
						 scaleY:(int)scaleY
					  inputNode:(MPSNNImageNode *__nonnull)inputNode {
	@autoreleasepool{
		self = [self init];

		mName = name;
		mUpsamplingNode = [MPSCNNUpsamplingNearestNode nodeWithSource:inputNode integerScaleFactorX:scaleY integerScaleFactorY:scaleY];
	
		return self;
	}
}

- (MPSNNImageNode *__nonnull) resultImage {
	return mUpsamplingNode.resultImage;
}

@end
