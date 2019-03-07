//
//  MLKConverterLib.mm
//  CoreML
//
//  Created by Sohaib Qureshi on 2017-01-13.
//  Copyright © 2017 Apple Inc. All rights reserved.
//

#include "Caffe/CaffeConverter.hpp"
#include "CaffeConverterLib.hpp"
#include "MLModelSpecification.hpp"

#include <sys/stat.h>

using namespace CoreML;

# pragma mark - Caffe Converter model

void convertCaffe(const std::string& srcPath,
                  const std::string& dstPath,
                  const std::map<std::string, std::string>& meanImageProtoPath,
                  const std::set<std::string>& imageInputs,
                  const std::map<std::string, bool>& isBGR,
                  const std::map<std::string, double>& redBias,
                  const std::map<std::string, double>& blueBias,
                  const std::map<std::string, double>& greenBias,
                  const std::map<std::string, double>& grayBias,
                  const std::map<std::string, double>& scale,
                  const std::string& caffeProtoTxtPath,
                  const std::string& classLabelPath,
                  const std::string& predictedFeatureName) {

    if (srcPath == "") {
        throw std::runtime_error("Required source model path --srcModelPath\n");
    }

    if (dstPath == "") {
        throw std::runtime_error("Required destination model path --dstModelPath\n");
    }

    struct stat buf;
    int statResult = stat(srcPath.c_str(), &buf);
    if (statResult != 0) {
        std::stringstream ss;
        ss << "Unable to open caffe model provided in the source model path: ";
        ss << srcPath;
        ss << std::endl;
        throw std::runtime_error(ss.str());
    }

    // Load the caffe network
    caffe::NetParameter caffeNetwork;
    caffe::NetParameter caffeWeightsNetwork;
    std::map<std::string, caffe::BlobProto> caffeMeanImageBlob;

    // TODO: We need to use only one caffe network proto variable (rdar://problem/30400140)
    // This is a workaround for that.
    if (caffeProtoTxtPath != "") {
        CoreMLConverter::loadCaffeNetwork(srcPath, caffeWeightsNetwork, caffeProtoTxtPath, caffeNetwork, meanImageProtoPath, caffeMeanImageBlob);

    } else {
        CoreMLConverter::loadCaffeNetwork(srcPath, caffeWeightsNetwork, caffeProtoTxtPath, caffeNetwork, meanImageProtoPath, caffeMeanImageBlob);
        caffeNetwork = caffeWeightsNetwork;
    }

    if (classLabelPath != "") {
	statResult = stat(classLabelPath.c_str(), &buf);
        if (statResult != 0) {
            std::stringstream ss;
            ss << "Unable to open class label file provided in the path: ";
            ss << classLabelPath;
            ss << std::endl;
            throw std::runtime_error(ss.str());
        }
    }

    // Convert the caffe network
    Specification::Model modelSpec;
    CoreMLConverter::convertCaffeNetwork(caffeWeightsNetwork, caffeNetwork, caffeMeanImageBlob, modelSpec,
                    isBGR, redBias, blueBias, greenBias, grayBias, scale, imageInputs, classLabelPath,
                    predictedFeatureName);

    // Save the format to the model path.
    Result r = saveSpecificationPath(modelSpec, dstPath);
    if (!r.good()) {
        throw std::runtime_error(r.message());
    }
}
