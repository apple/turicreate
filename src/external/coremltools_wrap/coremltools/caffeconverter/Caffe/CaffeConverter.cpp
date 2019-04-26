//  CoreML
//
//  Created by Srikrishna Sridhar on 11/13/16.
//  Copyright © 2016 Apple Inc. All rights reserved.
//

#include "CaffeConverter.hpp"
#include "UpgradeProto.hpp"
#include "Utils-inl.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <fcntl.h>

#include "MLModelSpecification.hpp"

using namespace CoreML;


// ----------------------------------------------------------------------------
//
//                Caffe converter utils
//
// ----------------------------------------------------------------------------

const int kProtoReadBytesLimit = INT_MAX;  // Read upto 2GB proto files.

/*
 * Caffe layer types are stored as "strings" in the protobuf. This will return
 * the conversion enum from string.
 *
 * @param[in]  layerType Layer type as stored by Caffe.
 * @return Layer type enum.
*/
CoreMLConverter::convertCaffeLayerFn
static caffeLayerRegistry(const std::string& layerType) {

    const std::unordered_map<std::string, CoreMLConverter::convertCaffeLayerFn> CAFFE_LAYER_REGISTRY {
        {"ReLU", CoreMLConverter::convertCaffeActivation},
        {"BNLL", CoreMLConverter::convertCaffeActivation},
        {"PReLU", CoreMLConverter::convertCaffeActivation},
        {"ELU", CoreMLConverter::convertCaffeActivation},
        {"Sigmoid", CoreMLConverter::convertCaffeActivation},
        {"TanH", CoreMLConverter::convertCaffeActivation},
        {"Parameter", CoreMLConverter::convertCaffeParameter},
        {"InnerProduct", CoreMLConverter::convertCaffeInnnerProduct},
        {"Convolution", CoreMLConverter::convertCaffeConvolution},
        {"Deconvolution", CoreMLConverter::convertCaffeConvolution},
        {"LRN", CoreMLConverter::convertCaffeLRN},
        {"Softmax", CoreMLConverter::convertCaffeSoftmax},
        {"Pooling", CoreMLConverter::convertCaffePooling},
        {"Concat", CoreMLConverter::convertCaffeConcat},
        {"LSTM", CoreMLConverter::convertCaffeLSTM},
        {"Eltwise",CoreMLConverter::convertCaffeEltwise},
        {"Slice",CoreMLConverter::convertCaffeSlice},
        {"Flatten",CoreMLConverter::convertCaffeFlatten},
        {"Embed",CoreMLConverter::convertCaffeEmbed},
        {"Split",CoreMLConverter::convertCaffeSplit},
        {"BatchNorm",CoreMLConverter::convertCaffeBatchnorm},
        {"Input",CoreMLConverter::convertCaffeInputLayers},
        {"ImageData",CoreMLConverter::convertCaffeInputLayers},
        {"ImageDataMultiLabel",CoreMLConverter::convertCaffeInputLayers},
        {"Data",CoreMLConverter::convertCaffeInputLayers},
        {"HDF5Data",CoreMLConverter::convertCaffeInputLayers},
        {"WindowData",CoreMLConverter::convertCaffeInputLayers},
        {"MemoryData",CoreMLConverter::convertCaffeInputLayers},
        {"DummyData",CoreMLConverter::convertCaffeInputLayers},
        {"HDF5Output",CoreMLConverter::convertCaffeTrainingLayers},
        {"Loss",CoreMLConverter::convertCaffeTrainingLayers},
        {"InfogainLoss",CoreMLConverter::convertCaffeTrainingLayers},
        {"EuclideanLoss",CoreMLConverter::convertCaffeTrainingLayers},
        {"Dropout",CoreMLConverter::convertCaffeTrainingLayers},
        {"SigmoidCrossEntropyLoss",CoreMLConverter::convertCaffeTrainingLayers},
        {"HingeLoss",CoreMLConverter::convertCaffeTrainingLayers},
        {"HingeLossMultiLabel",CoreMLConverter::convertCaffeTrainingLayers},
        {"Accuracy",CoreMLConverter::convertCaffeTrainingLayers},
        {"ContrastiveLoss",CoreMLConverter::convertCaffeTrainingLayers},
        {"SoftmaxWithLoss",CoreMLConverter::convertCaffeTrainingLayers},
        {"Python",CoreMLConverter::convertCaffeTrainingLayers},
        {"Reduction",CoreMLConverter::convertCaffeReduction},
        {"Scale",CoreMLConverter::convertCaffeScale},
        {"Bias",CoreMLConverter::convertCaffeBias},
        {"MVN",CoreMLConverter::convertCaffeMVN},
        {"AbsVal",CoreMLConverter::convertCaffeAbs},
        {"Exp",CoreMLConverter::convertCaffeExp},
        {"Power",CoreMLConverter::convertCaffePower},
        {"Log",CoreMLConverter::convertCaffeLog},
        {"Crop",CoreMLConverter::convertCaffeCrop},
        {"Reshape",CoreMLConverter::convertCaffeReshape}
    };

    // Find the layer in the global registry.
    const auto& it = CAFFE_LAYER_REGISTRY.find(layerType);
    if (it != CAFFE_LAYER_REGISTRY.end()) {
        return it->second;

    // Layer type not supported.
    } else {
        std::stringstream ss;
        ss << "Cannot convert caffe layer of type '" << layerType << "'." << std::endl;
        throw std::runtime_error(ss.str());
    }
}


// ----------------------------------------------------------------------------
//
//                           Caffe converter
//
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//                           Load Function
// ----------------------------------------------------------------------------
void CoreMLConverter::loadCaffeNetwork(const std::string& srcPathWeights,
                                      caffe::NetParameter& caffeSpecWeights,
                                      const std::string& srcPathProto,
                                      caffe::NetParameter& caffeSpecProto,
                                      const std::map<std::string, std::string>& meanImagePathProto,
                                      std::map<std::string, caffe::BlobProto>& meanImageBlobProto) {

    // Load the Caffemodel weights file
    std::ifstream iarc(srcPathWeights, std::ios::binary);
    google::protobuf::io::IstreamInputStream raw_input(&iarc);
    google::protobuf::io::CodedInputStream* coded_input = new google::protobuf::io::CodedInputStream(&raw_input);

    // Load in streams because these network files are going to be large and
    // protobuf default reader fails if messages are larger than 64MB.
    coded_input->SetTotalBytesLimit(kProtoReadBytesLimit, 536870912);
    bool openSuccess = caffeSpecWeights.ParseFromCodedStream(coded_input);
    if (!openSuccess) {
        std::stringstream msg;
        msg << "Unable to load caffe network Weights file: ";
        msg << srcPathWeights << ". ";
        throw std::runtime_error(msg.str());
    }
    CoreMLConverter::upgradeCaffeNetworkIfNeeded(srcPathWeights, caffeSpecWeights);
    delete coded_input;

    // Load the caffe prototxt file if applicable
    if (not srcPathProto.empty()) {
        int fileDescriptor = open(srcPathProto.c_str(), O_RDONLY);
        google::protobuf::io::FileInputStream fileInput(fileDescriptor);
        fileInput.SetCloseOnDelete( true );
        openSuccess = google::protobuf::TextFormat::Parse(&fileInput, &caffeSpecProto);
        if (!openSuccess) {
            std::stringstream msg;
            msg << "Unable to load caffe network Prototxt file: ";
            msg << srcPathProto;
            throw std::runtime_error(msg.str());
        }
        CoreMLConverter::upgradeCaffeNetworkIfNeeded(srcPathProto, caffeSpecProto);
    }

    // Load the binary proto file if available
    for (const auto& imageProto : meanImagePathProto) {
        std::ifstream iarc_mean_image(imageProto.second, std::ios::binary);
        google::protobuf::io::IstreamInputStream raw_input_mean_image(&iarc_mean_image);
        coded_input = new google::protobuf::io::CodedInputStream(&raw_input_mean_image);
        coded_input->SetTotalBytesLimit(kProtoReadBytesLimit, 536870912);
        openSuccess = meanImageBlobProto[imageProto.first].ParseFromCodedStream(coded_input);
        if (!openSuccess) {
            std::stringstream msg;
            msg << "Unable to load caffe network mean image binary proto file: ";
            msg << imageProto.second;
            throw std::runtime_error(msg.str());
        }
        delete coded_input;
    }

}

// ----------------------------------------------------------------------------
//                           Convert Function
// ----------------------------------------------------------------------------

template<typename T>
static void _addClassifierParameters(T* nnWrite,
                                     const std::set<std::string>& networkOutputs,
                                     const std::string& classInputPath,
                                     const std::string& predictedFeatureName,
                                     Specification::ModelDescription *modelInterface) {
    // by default, do nothing
#pragma unused(nnWrite)
#pragma unused(networkOutputs)
#pragma unused(classInputPath)
#pragma unused(predictedFeatureName)
#pragma unused(modelInterface)
}

template<>
void _addClassifierParameters(CoreML::Specification::NeuralNetworkClassifier* nnWrite,
                              const std::set<std::string>& networkOutputs,
                              const std::string& classInputPath,
                              const std::string& predictedFeatureName,
                              Specification::ModelDescription *modelInterface) {
    // Set the classifier classes
    if (networkOutputs.size() != 1) {
        std::stringstream ss;
        ss << "Model should have exactly one output (the probabilities) to automatically make it a classifier." << std::endl;
        throw std::runtime_error(ss.str());
    }

    // Add class labels
    std::string predictedProbabilitiesName = *(networkOutputs.begin());
    modelInterface->set_predictedprobabilitiesname(predictedProbabilitiesName);

    // Setting predictedFeatureName is required, but it will get filled in automatically and
    // doesn't need to be a model output explicitly.
    modelInterface->set_predictedfeaturename(predictedFeatureName);

    modelInterface->mutable_output(0)->mutable_type()->mutable_dictionarytype();
    modelInterface->mutable_output(0)->mutable_type()->mutable_dictionarytype()->mutable_stringkeytype();
    std::string class_buffer;
    std::ifstream infile;
    infile.open(classInputPath);
    while(getline(infile, class_buffer)) {
        auto *class_ = nnWrite->mutable_stringclasslabels()->add_vector();
        *class_ = class_buffer;
    }
    infile.close();

    // Add predicted class name
    Specification::FeatureDescription* classLabel = modelInterface->add_output();
    classLabel->set_name(predictedFeatureName);
    classLabel->mutable_type()->mutable_stringtype();
    modelInterface->set_predictedfeaturename(predictedFeatureName);
}

template<typename T>
static void _convertCaffeNetwork(caffe::NetParameter& caffeSpecWeights,
                                 caffe::NetParameter& caffeSpecProto,
                                 std::map<std::string, caffe::BlobProto>& meanImageBlobProtoAll,
                                 Specification::Model& modelSpec,
                                 const std::set<std::string>& imageInputs,
                                 const std::map<std::string, bool>& isBGR,
                                 const std::map<std::string, double>& redBias,
                                 const std::map<std::string, double>& blueBias,
                                 const std::map<std::string, double>& greenBias,
                                 const std::map<std::string, double>& grayBias,
                                 const std::map<std::string, double>& scale,
                                 const std::string& classInputPath,
                                 const std::string& predictedFeatureName,
                                 T* nnWrite) {

    modelSpec.set_specificationversion(MLMODEL_SPECIFICATION_VERSION);

    //Hash to map input/output names, to handle Split node and for skipping training layers.
    std::map<std::string, std::string> mappingDataBlobNames;

    // Maps the data blob name to its size. Currently only used for the Input Layer of Caffe.
    std::map<std::string, std::vector<int64_t> > mapBlobNameToDimensions;

    //map from caffe layer name to its index in the caffeSpecWeights message
    std::map<std::string, int> mapCaffeLayerNamesToIndex;
    for (int layer_id = 0; layer_id < caffeSpecWeights.layer_size(); layer_id++){
        if (caffeSpecWeights.layer(layer_id).has_name()){
            mapCaffeLayerNamesToIndex[caffeSpecWeights.layer(layer_id).name()] = layer_id;
        }
    }

    //caffe input names to the input layers
    std::set<std::string> caffeNetworkInputNames;

    int numberOfLayers = caffeSpecProto.layer_size();

    std::cout<<std::endl<<"================= Starting Conversion from Caffe to CoreML ======================"<<std::endl;
    for (int layer_id = 0; layer_id < numberOfLayers; layer_id++){

        const caffe::LayerParameter& caffeLayer = caffeSpecProto.layer(layer_id);

        //Check if the layer has type, name, top
        CoreMLConverter::validateCaffeLayerTypeAndName(caffeLayer);

        //************************************************************
        //print layer info
        int number_inputs = caffeLayer.bottom_size();
        int number_outputs = caffeLayer.top_size();
        std::cout << "Layer " << layer_id << ": Type: '" << caffeLayer.type() <<"', Name: '" << caffeLayer.name() << "'. ";
        if(number_inputs > 0){
            std::cout<<"Input(s): ";
            for(int i=0;i<number_inputs;i++){
                if(i>0){std::cout<<", ";}
                std::cout<< "'" << caffeLayer.bottom(i) << "'";
            }
            std::cout << ". ";
        }
        if(number_outputs > 0){
            std::cout<<"Output(s): ";
            for(int i=0;i<number_outputs;i++){
                if(i>0){std::cout<<", ";}
                std::cout<< "'" << caffeLayer.top(i) << "'";
            }
            std::cout << "." << std::endl;
        }
        //************************************************************

        //Call the layer convert function
        CoreMLConverter::convertCaffeLayerFn layerConvertFn = caffeLayerRegistry(caffeLayer.type());

        CoreMLConverter::ConvertLayerParameters layerParams(caffeSpecProto,
                                                           caffeSpecWeights,
                                                           mappingDataBlobNames,
                                                           mapCaffeLayerNamesToIndex,
                                                           mapBlobNameToDimensions,
                                                           caffeNetworkInputNames);
        layerParams.nnWrite = nnWrite->mutable_layers();
        layerParams.layerId = &layer_id;
        layerConvertFn(layerParams);

    } //end of looping over caffe layers

    std::cout<<std::endl<<"================= Summary of the conversion: ==================================="<<std::endl;

    //if mapBlobNameToDimensions is empty, this means we have not been able to infer input dimensions.
    if (mapBlobNameToDimensions.size() == 0){
        std::stringstream ss;
        ss << "Unable to infer input name and dimensions. ";
        ss << "Please provide a .prototxt file with 'Input' layer and dimensions defined." << std::endl;
        throw std::runtime_error(ss.str());
    } else {
        std::cout<<"Detected input(s) and shape(s) (ignoring batch size):"<<std::endl;
        for (const auto& input: mapBlobNameToDimensions){
            std::cout<<"'"<<input.first<<"' : ";
            std::vector<int64_t> dims = mapBlobNameToDimensions.at(input.first);
            for (size_t i=0; i<dims.size(); i++){
                if (i>0) {std::cout << ", ";}
                std::cout<<dims[i];
            }
            std::cout<<std::endl;
        }
    }

    // We'll create sets of all input and output names
    std::set<std::string> inputNames;
    std::set<std::string> outputNames;
    for (const auto& nnLayer: nnWrite->layers()) {
        // layers can possibly have multiple inputs and outputs, so we loop over them here
        for (const auto& layerName: nnLayer.input()) {
            inputNames.insert(layerName);
        }
        for (const auto& layerName: nnLayer.output()) {
            outputNames.insert(layerName);
        }
    }

    // Now, the inputs to the whole network are the set difference between inputs and outputs
    // in other words, all internal nodes appear in both lists
    // We're using a pair which is the name, followed by the dimensions of the input/output
    std::set<std::string> networkInputs;
    std::set_difference(inputNames.begin(), inputNames.end(), outputNames.begin(), outputNames.end(),
                        inserter(networkInputs, networkInputs.begin()));

    // add any input that might have been left out in "networkInputs" because it is "dangling", i.e not feeding in to any other layer.
    // This is not ideal, but an error should not be raised if this happens, instead the user must be warned.
    for (const auto& networkInputName:caffeNetworkInputNames){
        if (networkInputs.find(networkInputName) == networkInputs.end()){
            networkInputs.insert(networkInputName);
            std::stringstream ss;
            ss <<"WARNING: The input: '"<<networkInputName<< "', is dangling i.e. it does not feed into any other layer of the network." << std::endl;
            std::cout << ss.str();
        }
    }

    // Similarly for the output names
    std::set<std::string> networkOutputs;
    std::set_difference(outputNames.begin(), outputNames.end(), inputNames.begin(), inputNames.end(),
                        inserter(networkOutputs, networkOutputs.begin()));

    // copy the input names into the proto
    Specification::ModelDescription *modelInterface = modelSpec.mutable_description();
    if (networkInputs.size() == 0) {
        throw std::runtime_error("Unable to find any input layer for the network.");
    }

    for (const auto& inputName: networkInputs) {
        Specification::FeatureDescription *inputDesc = modelInterface->add_input();
        inputDesc->set_name(inputName);
        Specification::FeatureType *inputType = inputDesc->mutable_type();

        // Raise error if we haven't been able to determine dimensions of the detected inputs
        if (mapBlobNameToDimensions.find(inputName) == mapBlobNameToDimensions.end()){
            std::stringstream ss;
            ss <<"Unable to infer shape for the Input '"<<inputName<<"'."<<std::endl;
            throw std::runtime_error(ss.str());
        }

        if ((imageInputs.find(inputName)) != imageInputs.end()) {

            Specification::ImageFeatureType *params = inputType->mutable_imagetype();
            // now we assume that the input dimensions are channels, height, width
            if (mapBlobNameToDimensions[inputName][0]==1) {
                params->set_colorspace(Specification::ImageFeatureType_ColorSpace_GRAYSCALE);
            } else if (isBGR.find(inputName) != isBGR.end()) {
                if (isBGR.at(inputName)){
                    params->set_colorspace(Specification::ImageFeatureType_ColorSpace_BGR);
                } else {
                    params->set_colorspace(Specification::ImageFeatureType_ColorSpace_RGB);
                }
            } else {
                params->set_colorspace(Specification::ImageFeatureType_ColorSpace_RGB);
            }
            params->set_height(mapBlobNameToDimensions[inputName][1]);
            params->set_width(mapBlobNameToDimensions[inputName][2]);
            Specification::NeuralNetworkPreprocessing* preprocessing = nnWrite->mutable_preprocessing()->Add();

            if (meanImageBlobProtoAll.find(inputName) != meanImageBlobProtoAll.end()){

                caffe::BlobProto meanImageBlobProto = meanImageBlobProtoAll.at(inputName);

                if (meanImageBlobProto.data_size() == 0){
                    std::stringstream ss;
                    ss <<"There is no data in the mean image binary proto file specified for input: '"<< inputName << "'."<< std::endl;
                    throw std::runtime_error(ss.str());
                }

                int dataSize = meanImageBlobProto.data_size();
                int C = static_cast<int>(mapBlobNameToDimensions[inputName][0]);
                int H = static_cast<int>(mapBlobNameToDimensions[inputName][1]);
                int W = static_cast<int>(mapBlobNameToDimensions[inputName][2]);
                //get the mean Image proto size:
                int CMeanImage, HMeanImage, WMeanImage;
                if (meanImageBlobProto.shape().dim_size() == 0){
                    CMeanImage = meanImageBlobProto.channels();
                    HMeanImage = meanImageBlobProto.height();
                    WMeanImage = meanImageBlobProto.width();
                    if (CMeanImage == 0 || HMeanImage == 0 || WMeanImage == 0){
                        std::stringstream ss;
                        ss <<"Shape of mean image (C, H, W) in binary proto cannot be 0. " << std::endl;
                        throw std::runtime_error(ss.str());
                    }
                } else if (!(meanImageBlobProto.shape().dim_size() == 3 ||
                             (meanImageBlobProto.shape().dim_size() == 2 && C == 1))) {
                    std::stringstream ss;
                    ss <<"Shape of mean image in binary proto must be either 2D (grayscale) [H,W] or 3D [C,H,W]. " << std::endl;
                    throw std::runtime_error(ss.str());
                } else {
                    if (meanImageBlobProto.shape().dim_size() == 3) {
                        CMeanImage = static_cast<int>(meanImageBlobProto.shape().dim(0));
                        HMeanImage = static_cast<int>(meanImageBlobProto.shape().dim(1));
                        WMeanImage = static_cast<int>(meanImageBlobProto.shape().dim(2));
                    } else {
                        CMeanImage = 1;
                        HMeanImage = static_cast<int>(meanImageBlobProto.shape().dim(0));
                        WMeanImage = static_cast<int>(meanImageBlobProto.shape().dim(1));
                    }
                }
                if (dataSize !=  CMeanImage * HMeanImage * WMeanImage){
                    std::stringstream ss;
                    ss <<"Size of data in mean image binary proto must be consistent with its shape (C,H,W). " << std::endl;
                    throw std::runtime_error(ss.str());
                }
                if (HMeanImage < H || WMeanImage < W){
                    std::stringstream ss;
                    ss <<"Height and width of the mean image must be greater than or equal to the input image size. " << std::endl;
                    throw std::runtime_error(ss.str());
                }

                preprocessing->set_featurename(inputName);
                Specification::NeuralNetworkMeanImage* meanImage = preprocessing->mutable_meanimage();
                ::google::protobuf::RepeatedField<float>* meanImageWrite = meanImage->mutable_meanimage(); //destination
                meanImageWrite->Resize(C * H * W, 0.0);

                if (HMeanImage > H || WMeanImage > W){
                    std::stringstream ss;
                    ss <<"Size of mean image: (H,W) = (" << HMeanImage << ", " << WMeanImage << ") is greater than input image size: (H,W) = (";
                    ss << H << ", " << W << "). Mean image will be center cropped to match the input image dimensions. " << std::endl;
                    std::cout << ss.str();

                    //center crop the mean image:
                    int Hoffset = (HMeanImage - H)/2;
                    int Woffset = (WMeanImage - W)/2;
                    for (int i = 0; i < C; i++){
                        for (int j = 0; j < H; j++){
                            for (int k = 0; k < W; k++){
                                meanImageWrite->Set(i * H * W + j * W + k, meanImageBlobProto.data(i * HMeanImage * WMeanImage + (Hoffset+j) * WMeanImage + Woffset+k));
                            }
                        }
                    }
                } else {
                    meanImageWrite->CopyFrom(meanImageBlobProto.data());
                }
            } else { //instead of mean image preprocessing we have scaler preprocessing
                preprocessing->set_featurename(inputName);
                Specification::NeuralNetworkImageScaler* scaler = preprocessing->mutable_scaler();
                if (scale.find(inputName) != scale.end()){
                    scaler->set_channelscale(static_cast<float>(scale.at(inputName)));
                } else {
                    scaler->set_channelscale(1.0f);
                }
                if (redBias.find(inputName) != redBias.end()) {
                    scaler->set_redbias(static_cast<float>(redBias.at(inputName)));
                }
                if (greenBias.find(inputName) != greenBias.end()){
                    scaler->set_greenbias(static_cast<float>(greenBias.at(inputName)));
                }
                if (blueBias.find(inputName) != blueBias.end()){
                    scaler->set_bluebias(static_cast<float>(blueBias.at(inputName)));
                }
                if (grayBias.find(inputName) != grayBias.end()){
                    scaler->set_graybias(static_cast<float>(grayBias.at(inputName)));
                }
            }
        }
        else {

            // fill in the input sizes
            Specification::ArrayFeatureType *array = inputType->mutable_multiarraytype();
            array->set_datatype(::CoreML::Specification::ArrayFeatureType_ArrayDataType::ArrayFeatureType_ArrayDataType_DOUBLE);
            const std::vector<int64_t>& dims = mapBlobNameToDimensions[inputName];
            for (const int64_t& val: dims) {
                array->add_shape(val);
            }

        }

    } // end of loop over all network inputs


    // Likewise, we'll fill out the output names
    for (const auto& outputName: networkOutputs) {
        Specification::FeatureDescription *outputDesc = modelInterface->add_output();
        outputDesc->set_name(outputName);
        Specification::FeatureType *outputType = outputDesc->mutable_type();
        Specification::ArrayFeatureType *array = outputType->mutable_multiarraytype();
        array->set_datatype(::CoreML::Specification::ArrayFeatureType_ArrayDataType::ArrayFeatureType_ArrayDataType_DOUBLE);
    }
    std::cout << std::endl;
    std::cout << "Network Input name(s): ";
    int iter = 0;
    for (const auto& name: networkInputs){
        if (iter>0) {std::cout << ", ";}
        iter++;
        std::cout << "'" << name << "'";
    }
    std::cout << "." << std::endl;
    std::cout << "Network Output name(s): ";
    iter = 0;
    for (const auto& name: networkOutputs){
        if (iter>0) {std::cout << ", ";}
        iter++;
        std::cout << "'" << name << "'";
    }
    std::cout << "." << std::endl << std::endl;

    // will be a noop for any types other than NeuralNetworkClassifier
    _addClassifierParameters(nnWrite,
                             networkOutputs,
                             classInputPath,
                             predictedFeatureName,
                             modelInterface);
}


void CoreMLConverter::convertCaffeNetwork(caffe::NetParameter& caffeSpecWeights,
                                          caffe::NetParameter& caffeSpecProto,
                                          std::map<std::string, caffe::BlobProto>& meanImageBlobProto,
                                          Specification::Model& modelSpec,
                                          const std::map<std::string, bool>& isBGR,
                                          const std::map<std::string, double>& redBias,
                                          const std::map<std::string, double>& blueBias,
                                          const std::map<std::string, double>& greenBias,
                                          const std::map<std::string, double>& grayBias,
                                          const std::map<std::string, double>& scale,
                                          const std::set<std::string>& imageInputs,
                                          const std::string& classInputPath,
                                          const std::string& predictedFeatureName) {


    // Currently, we don't support caffe V1 or V0.
    if (caffeSpecProto.layers_size() != 0) {
        throw std::runtime_error("Caffe prototxt file is not version 2. Please save this model using Caffe V2.");
    }
    if (caffeSpecWeights.layers_size() != 0) {
        throw std::runtime_error("Caffemodel file is not version 2. Please save this model using Caffe V2.");
    }

    if (classInputPath != "") {
        // we have class labels, produce a classifier
        _convertCaffeNetwork(caffeSpecWeights,
                             caffeSpecProto,
                             meanImageBlobProto,
                             modelSpec,
                             imageInputs,
                             isBGR,
                             redBias,
                             blueBias,
                             greenBias,
                             grayBias,
                             scale,
                             classInputPath,
                             predictedFeatureName,
                             modelSpec.mutable_neuralnetworkclassifier());
    } else {
        _convertCaffeNetwork(caffeSpecWeights,
                             caffeSpecProto,
                             meanImageBlobProto,
                             modelSpec,
                             imageInputs,
                             isBGR,
                             redBias,
                             blueBias,
                             greenBias,
                             grayBias,
                             scale,
                             classInputPath,
                             predictedFeatureName,
                             modelSpec.mutable_neuralnetwork());
    }
}
