#ifndef MLMODEL_UTILS
#define MLMODEL_UTILS

#include <fstream>
#include <memory>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "Globals.hpp"
#include "Model.hpp"
#include "Result.hpp"
#include "Format.hpp"

namespace CoreML {

    // This is the type used internally in Espresso for float 16
    typedef unsigned short float16;
    
    // insert_or_assign not available until C++17
    template<typename K, typename V>
    inline void insert_or_assign(std::unordered_map<K, V>& map, const K& k, const V& v) {
        const auto result = map.find(k);
        if (result == map.end()) {
            // insert case
            map.insert(std::make_pair(k, v));
        } else {
            // assign case
            V& existing = map.at(k);
            existing = v;
        }
    }
   
    template <typename T>
    static inline Result saveSpecification(const T& formatObj,
                                           std::ostream& out) {
        google::protobuf::io::OstreamOutputStream rawOutput(&out);

        if (!formatObj.SerializeToZeroCopyStream(&rawOutput)) {
            return Result(ResultType::FAILED_TO_SERIALIZE,
                          "unable to serialize object");
        }

        return Result();
    }


    static inline Result saveSpecificationPath(const Specification::Model& formatObj,
                                               const std::string& path) {
        Model m(formatObj);
        return m.save(path);
    }

    template <typename T>
    static inline  Result loadSpecification(T& formatObj,
                                            std::istream& in) {

        google::protobuf::io::IstreamInputStream rawInput(&in);
        google::protobuf::io::CodedInputStream codedInput(&rawInput);

        codedInput.SetTotalBytesLimit(std::numeric_limits<int>::max(), -1);

        if (!formatObj.ParseFromCodedStream(&codedInput)) {
            return Result(ResultType::FAILED_TO_DESERIALIZE,
                          "unable to deserialize object");
        }
        
        return Result();
    }
    
    static inline Result loadSpecificationPath(Specification::Model& formatObj,
                                               const std::string& path) {
        Model m;
        Result r = CoreML::Model::load(path, m);
        if (!r.good()) { return r; }
        formatObj = m.getProto();
        return Result();
    }
    
    // Helper functions for determining model version
    bool hasCustomLayer(Specification::Model& model);

    bool hasOnlyFlexibleShapes(Specification::Model& model);
    bool hasIOS12Features(Specification::Model& model);

    typedef std::pair<std::string,std::string> StringPair;
    // Returns a vector of pairs of strings, one pair per custom layer instance
    std::vector<StringPair> getCustomLayerNamesAndDescriptions(const Specification::Model& model);
    // Returns a vector of pairs of strings, one pair per custom model instance
    std::vector<StringPair> getCustomModelNamesAndDescriptions(const Specification::Model& model);

    bool hasfp16Weights(Specification::Model& model);

    bool hasCustomModel(Specification::Model& model);
    bool hasAppleWordTagger(Specification::Model& model);
    bool hasAppleTextClassifier(Specification::Model& model);
    bool hasAppleImageFeatureExtractor(Specification::Model& model);
    bool hasCategoricalSequences(Specification::Model& model);
    
    // Returns the lowest precision weight type among the weights in the layer
    WeightParamType getLSTMWeightParamType(const Specification::LSTMWeightParams& params);
    WeightParamType getWeightParamType(const Specification::NeuralNetworkLayer& layer);

    static inline std::vector<float16> readFloat16Weights(const Specification::WeightParams& weights) {

        std::string weight_bytes = weights.float16value();
        std::vector<float16> output(weight_bytes.size() / 2);
        
        for (size_t i = 0; i < weight_bytes.size(); i+=2) {

            float16 out = static_cast<float16>((static_cast<float16>(weight_bytes[i]) << 8)) | static_cast<float16>(weight_bytes[i+1]);
            output[i/2] = out;
            
        }
        return output;
    }
    
}

google::protobuf::RepeatedPtrField<CoreML::Specification::NeuralNetworkLayer> const *getNNSpec(const CoreML::Specification::Model& model);

#endif
