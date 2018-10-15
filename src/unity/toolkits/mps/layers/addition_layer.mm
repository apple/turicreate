#import <unity/toolkits/mps/layers/addition_layer.h>

@implementation AdditionLayer 

- (id __nonnull) initWithParams:(NSString * __nonnull)name
					   leftNode:(MPSNNImageNode * __nonnull)leftNode
					  rightNode:(MPSNNImageNode * __nonnull)rightNode {
	@autoreleasepool {
		self = [self init];
		mName = name;
		mAdditionNode = [MPSNNAdditionNode nodeWithLeftSource:leftNode
												  rightSource:rightNode];
		return self;
	}
}

- (MPSNNImageNode * _Nonnull) resultImage {
	return mAdditionNode.resultImage;
}

@end
