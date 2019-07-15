#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

@interface TCMPSConvolutionDescriptor:NSObject
@property (nonatomic) NSUInteger kernelWidth;
@property (nonatomic) NSUInteger kernelHeight;
@property (nonatomic) NSUInteger inputFeatureChannels;
@property (nonatomic) NSUInteger outputFeatureChannels;
@property (nonatomic) NSUInteger strideWidth;
@property (nonatomic) NSUInteger strideHeight;
@property (nonatomic) NSUInteger paddingWidth;
@property (nonatomic) NSUInteger paddingHeight;
@property (nonatomic, copy) NSString *label;
@property (nonatomic) BOOL updateWeights;
@end

@interface TCMPSInstanceNormalizationDescriptor:NSObject
@property (nonatomic) NSUInteger channels;
@property (nonatomic) NSUInteger styles;
@property (nonatomic, copy) NSString *label;
@end

@interface TCMPSUpsamplingDescriptor:NSObject
@property (nonatomic) NSUInteger scale;
@end

@interface TCMPSPoolingDescriptor:NSObject
@property (nonatomic) NSUInteger kernelSize;
@property (nonatomic) NSUInteger strideSize;
@end

@interface TCMPSEncodingDescriptor:NSObject
@property (nonatomic) TCMPSConvolutionDescriptor* conv;
@property (nonatomic) TCMPSInstanceNormalizationDescriptor* inst;
@end

@interface TCMPSResidualDescriptor:NSObject
@property (nonatomic) TCMPSConvolutionDescriptor* conv1;
@property (nonatomic) TCMPSConvolutionDescriptor* conv2;
@property (nonatomic) TCMPSInstanceNormalizationDescriptor* inst1;
@property (nonatomic) TCMPSInstanceNormalizationDescriptor* inst2;
@end

@interface TCMPSDecodingDescriptor:NSObject
@property (nonatomic) TCMPSConvolutionDescriptor* conv;
@property (nonatomic) TCMPSInstanceNormalizationDescriptor* inst;
@property (nonatomic) TCMPSUpsamplingDescriptor* upsample;
@end

@interface TCMPSVgg16Block1Descriptor:NSObject
@property (nonatomic) TCMPSConvolutionDescriptor* conv1;
@property (nonatomic) TCMPSConvolutionDescriptor* conv2;
@property (nonatomic) TCMPSPoolingDescriptor* pooling;
@end

@interface TCMPSVgg16Block2Descriptor:NSObject
@property (nonatomic) TCMPSConvolutionDescriptor* conv1;
@property (nonatomic) TCMPSConvolutionDescriptor* conv2;
@property (nonatomic) TCMPSConvolutionDescriptor* conv3;
@property (nonatomic) TCMPSPoolingDescriptor* pooling;
@end

@interface TCMPSVgg16Descriptor:NSObject
@property (nonatomic) TCMPSVgg16Block1Descriptor* block1;
@property (nonatomic) TCMPSVgg16Block1Descriptor* block2;
@property (nonatomic) TCMPSVgg16Block2Descriptor* block3;
@property (nonatomic) TCMPSVgg16Block2Descriptor* block4;
@end

@interface TCMPSTransformerDescriptor:NSObject
@property (nonatomic) TCMPSEncodingDescriptor* encode1;
@property (nonatomic) TCMPSEncodingDescriptor* encode2;
@property (nonatomic) TCMPSEncodingDescriptor* encode3;

@property (nonatomic) TCMPSResidualDescriptor* residual1;
@property (nonatomic) TCMPSResidualDescriptor* residual2;
@property (nonatomic) TCMPSResidualDescriptor* residual3;
@property (nonatomic) TCMPSResidualDescriptor* residual4;
@property (nonatomic) TCMPSResidualDescriptor* residual5;

@property (nonatomic) TCMPSDecodingDescriptor* decode1;
@property (nonatomic) TCMPSDecodingDescriptor* decode2;

@property (nonatomic) TCMPSConvolutionDescriptor* conv;
@property (nonatomic) TCMPSInstanceNormalizationDescriptor* inst;
@end