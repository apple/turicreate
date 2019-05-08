#import <unity/toolkits/drawing_classifier/mps_drawing_classifier.hpp>

@implementation DrawingClassifierModel

- (id _Nullable)initWithParameters:(NSString *_Nonnull)name
                         inputNode:(MPSNNImageNode *_Nonnull)inputNode
                            device:(id<MTLDevice> _Nonnull)dev
                         cmd_queue:(id<MTLCommandQueue> _Nonnull)cmd_q
                       initWeights:(std::map<std::string, std::vector<float>>)weights {
  @autoreleasepool {
    self = [super init];
    return self;
  }
}

@end