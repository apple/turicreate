//
//  MLKConverterLib.h
//  CoreML
//
//  Created by Sohaib Qureshi on 2017-01-13.
//  Copyright © 2017 Apple Inc. All rights reserved.
//

#include <map>
#include <string>
#include <set>

/*
 * Convert from the caffe protobuf format to the CoreML protobuf format.
 *
 * @param[in]  srcModelPath Caffe model format (.caffemodel)
 * @param[in]  dstModelPath mlmodel file path
 * @param[in]  meanImageProtoPath input names and paths to mean image binaryprotos.
 * @param[in]  imageInputs Blob names that must be treated as images.
 * @param[in]  input names and isBGR. Is the order of the data BGR?
 * @param[in]  input names and redBias Image bias value.
 * @param[in]  input names and blueBias Image bias value.
 * @param[in]  input names and greenBias Image bias value.
 * @param[in]  input names and grayBias Image bias value.
 * @param[in]  input names and channel scale value.
 * @param[in]  classLabelPath File where the class labels are encoded.
 * @param[in]  predictedFeatureName Name of the predicted feature.
 * \returns True if the conversion happened successfully.
 */
void convertCaffe(const std::string& srcModelPath,
                         const std::string& dstModelPath,
                         const std::map<std::string, std::string>& meanImageProtoPath = std::map<std::string, std::string>(),
                         const std::set<std::string>& imageInputs = std::set<std::string>(),
                         const std::map<std::string, bool>& isBGR = std::map<std::string, bool>(),
                         const std::map<std::string, double>& redBias = std::map<std::string, double>(),
                         const std::map<std::string, double>& blueBias = std::map<std::string, double>(),
                         const std::map<std::string, double>& greenBias = std::map<std::string, double>(),
                         const std::map<std::string, double>& grayBias = std::map<std::string, double>(),
                         const std::map<std::string, double>& scale = std::map<std::string, double>(),
                         const std::string& caffeProtoTxtPath = "",
                         const std::string& classLabelPath = "",
                         const std::string& predictedFeatureName = "");
