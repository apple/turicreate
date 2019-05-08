#ifndef TURI_MPS_DRAWING_CLASSIGFIER_H_
#define TURI_MPS_DRAWING_CLASSIGFIER_H_

#include <map>
#include <string>
#include <vector>

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

API_AVAILABLE(macos(10.14))
@interface DrawingClassifierModel : NSObject {
}

- (id _Nullable)initWithParameters:(NSString *_Nonnull)name
                         inputNode:(MPSNNImageNode *_Nonnull)inputNode
                            device:(id<MTLDevice> _Nonnull)dev
                         cmd_queue:(id<MTLCommandQueue> _Nonnull)cmd_q
                       initWeights:(std::map<std::string, std::vector<float>>)weights;
@end

#endif