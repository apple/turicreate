#import <unity/toolkits/mps/layers/input_layer.h>

@implementation InputLayer

- (id __nonnull) initInput {
	@autoreleasepool {
        self = [super init];
		mResult = [MPSNNImageNode nodeWithHandle: nil];
		return self;
	}
}

- (MPSNNImageNode *__nonnull) resultImage {
    return mResult;
}

@end