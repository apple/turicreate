//
//  CaffeConverter.hpp
//  CoreML
//
//  Created by Srikrishna Sridhar on 11/13/16.
//  Copyright © 2016 Apple Inc. All rights reserved.
//

#ifndef CaffeConverter_hpp
#define CaffeConverter_hpp

#include "caffe_pb_wrapper.hpp"
#include "MLModelSpecification.hpp"

#include <stdio.h>
#include <functional>

namespace CoreMLConverter {

    //struct to specify parameters taken by convert layer functions
    struct ConvertLayerParameters {
        const caffe::NetParameter& prototxt;
        const caffe::NetParameter& protoweights;
        ::google::protobuf::RepeatedPtrField< ::CoreML::Specification::NeuralNetworkLayer >* nnWrite;
        bool isClassifier;
        std::map<std::string, std::string>& mappingDataBlobNames;
        std::map<std::string, int>& mapCaffeLayerNamesToIndex;
        std::map<std::string, std::vector<int64_t> >& mapBlobNameToDimensions;
        std::set<std::string>& caffeNetworkInputNames;
        int* layerId;

        ConvertLayerParameters(const caffe::NetParameter& ptxt,
                               const caffe::NetParameter& pw,
                               std::map<std::string, std::string>& mapDataBlobs,
                               std::map<std::string, int>& layerToIndex,
                               std::map<std::string, std::vector<int64_t> >& blobToDim,
                               std::set<std::string>& networkInputNames):
                                    prototxt(ptxt),
                                    protoweights(pw),
                                    mappingDataBlobNames(mapDataBlobs),
                                    mapCaffeLayerNamesToIndex(layerToIndex),
                                    mapBlobNameToDimensions(blobToDim),
                                    caffeNetworkInputNames(networkInputNames) {}
    };

    typedef std::function<void(ConvertLayerParameters layerParameters)> convertCaffeLayerFn;

    /*
     * Load caffe network. Error out if not possible.
     *
     * @param[in] srcPath Path where the caffe network.
     * @param[out] caffeSpec Caffe model format
     */
    void loadCaffeNetwork(const std::string& srcPathWeights,
                          caffe::NetParameter& caffeSpecWeights,
                          const std::string& srcPathProto,
                          caffe::NetParameter& caffeSpecProto,
                          const std::map<std::string, std::string>& meanImagePathProto,
                          std::map<std::string, caffe::BlobProto>& meanImageBlobProto);

    /*
     * Convert from the caffe protobuf format to the CoreML protobuf format.
     *
     * @param[in]  caffeSpecWeights Caffe model format
     * @param[in]  caffeSpecProtoTxt Caffe model format
     * @param[out] modelSpec CoreML model spec.
     * @param[in]  imageNames Blob names that must be treated as images.
     * @param[in]  isBGR does the network expect data to be in order BGR (applicable only for images)?
     * @param[in]  redBias Image bias value. (applicable only for images)
     * @param[in]  blueBias Image bias value. (applicable only for images)
     * @param[in]  greenBias Image bias value. (applicable only for images)
     * @param[in]  grayBias Image bias value. (applicable only for images)
     * @param[in]  scale Image scale value. Same value across all channels. (applicable only for images)
     * @param[in]  classInputPath Path where classes are present (applicable only for classifiers)
    */
    void convertCaffeNetwork(caffe::NetParameter& caffeSpecWeights,
                             caffe::NetParameter& caffeSpecProtoTxt,
                             std::map<std::string, caffe::BlobProto>& meanImageBlobProto,
                             CoreML::Specification::Model& modelSpec,
                             const std::map<std::string, bool>& isBGR = std::map<std::string, bool>(),
                             const std::map<std::string, double>& redBias = std::map<std::string, double>(),
                             const std::map<std::string, double>& blueBias = std::map<std::string, double>(),
                             const std::map<std::string, double>& greenBias = std::map<std::string, double>(),
                             const std::map<std::string, double>& grayBias = std::map<std::string, double>(),
                             const std::map<std::string, double>& scale = std::map<std::string, double>(),
                             const std::set<std::string>& imageNames = std::set<std::string>(),
                             const std::string& classInputPath = "",
                             const std::string& predictedFeatureName = "");

    // Specializations for each layer.
    // ------------------------------------------------------------------------

    void convertCaffeInnnerProduct(ConvertLayerParameters layerParameters);
    void convertCaffeActivation(ConvertLayerParameters layerParameters);
    void convertCaffeLRN(ConvertLayerParameters layerParameters);
    void convertCaffeSoftmax(ConvertLayerParameters layerParameters);
    void convertCaffePooling(ConvertLayerParameters layerParameters);
    void convertCaffeConcat(ConvertLayerParameters layerParameters);
    void convertCaffeConvolution(ConvertLayerParameters layerParameters);
    void convertCaffeLSTM(ConvertLayerParameters layerParameters);
    void convertCaffeEltwise(ConvertLayerParameters layerParameters);
    void convertCaffeSlice(ConvertLayerParameters layerParameters);
    void convertCaffeEmbed(ConvertLayerParameters layerParameters);
    void convertCaffeFlatten(ConvertLayerParameters layerParameters);
    void convertCaffeSplit(ConvertLayerParameters layerParameters);
    void convertCaffeBatchnorm(ConvertLayerParameters layerParameters);
    void convertCaffeInputLayers(ConvertLayerParameters layerParameters);
    void convertCaffeTrainingLayers(ConvertLayerParameters layerParameters);
    void convertCaffeParameter(ConvertLayerParameters layerParameters);
    void convertCaffeReduction(ConvertLayerParameters layerParameters);
    void convertCaffeScale(ConvertLayerParameters layerParameters);
    void convertCaffeBias(ConvertLayerParameters layerParameters);
    void convertCaffeMVN(ConvertLayerParameters layerParameters);
    void convertCaffeAbs(ConvertLayerParameters layerParameters);
    void convertCaffeExp(ConvertLayerParameters layerParameters);
    void convertCaffePower(ConvertLayerParameters layerParameters);
    void convertCaffeLog(ConvertLayerParameters layerParameters);
    void convertCaffeCrop(ConvertLayerParameters layerParameters);
    void convertCaffeReshape(ConvertLayerParameters layerParameters);

}

#endif /* CaffeConverter_hpp */
