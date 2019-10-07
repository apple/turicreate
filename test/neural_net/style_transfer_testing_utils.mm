#include "style_transfer_testing_utils.hpp"

#include <cmath>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <iostream>
#include <string>

using boost::lexical_cast;
using boost::property_tree::ptree;
using boost::property_tree::read_json;

@implementation NeuralNetStyleTransferUtils

+ (NSData*) loadData:(NSString*)url {
  NSString *dataFile = [url stringByStandardizingPath];
  NSData *inData = [NSData dataWithContentsOfFile: dataFile];
  return inData;
}

+ (ptree) loadConfig:(NSString*)url {
  ptree root;
  std::string url_string = std::string([url UTF8String]);
  read_json(url_string, root);
  return root;
}

+ (MPSImageBatch *) defineInput:(NSString *)path
                            dev:(id<MTLDevice>)dev {
  if (@available(macOS 10.15, *)) {
    NSString *inputDataPath
    = [NSString stringWithFormat:@"%@/%@", path, @"input.bin"];
    NSData* inputData = [NeuralNetStyleTransferUtils loadData:inputDataPath];
    
    NSString *inputConfigPath
    = [NSString stringWithFormat:@"%@/%@", path, @"inputs.json"];
    ptree root = [NeuralNetStyleTransferUtils loadConfig:inputConfigPath];
    
    NSUInteger imageWidth  = root.get<NSUInteger>("width");
    NSUInteger imageHeight = root.get<NSUInteger>("height");
    NSUInteger imageChannels = root.get<NSUInteger>("channels");
    
    MPSImageDescriptor *imgDesc = [MPSImageDescriptor
                                   imageDescriptorWithChannelFormat:MPSImageFeatureChannelFormatFloat32
                                   width:imageWidth
                                   height:imageHeight
                                   featureChannels:imageChannels
                                   numberOfImages:1
                                   usage:MTLTextureUsageShaderWrite | MTLTextureUsageShaderRead];
    
    NSMutableArray<MPSImage *> *imageBatch = [[NSMutableArray alloc] init];
    
    MPSImage *image = [[MPSImage alloc] initWithDevice:dev
                                       imageDescriptor:imgDesc];
    
    [image writeBytes:inputData.bytes
           dataLayout:MPSDataLayoutHeightxWidthxFeatureChannels
           imageIndex:0];
    
    [imageBatch addObject:image];
    
    return imageBatch;
    
    
  } else {
    throw "Need to be on MacOS 10.15 to use this function";
  }
}

+ (NSData *) defineOutput:(NSString *)path {
  if (@available(macOS 10.15, *)) {
    NSString *outputDataPath
    = [NSString stringWithFormat:@"%@/%@", path, @"output.bin"];
    return [NeuralNetStyleTransferUtils loadData:outputDataPath];
  } else {
    throw "Need to be on MacOS 10.15 to use this function";
  }
}

+ (BOOL) checkData:(NSData *)expected
            actual:(NSData *)actual
           epsilon:(float)epsilon {
  if (@available(macOS 10.15, *)) {
    float* floatActual = (float *) actual.bytes;
    float* floatExpected = (float *) expected.bytes;
    
    for (NSUInteger x = 0; x < expected.length/sizeof(float) ; x++)
      if (std::abs(floatActual[x] - floatExpected[x]) > epsilon
          && !std::isnan(floatActual[x])
          && !std::isnan(floatExpected[x]))
        return false;
    
    return true;
  } else {
    throw "Need to be on MacOS 10.15 to use this function";
  }
}

@end
