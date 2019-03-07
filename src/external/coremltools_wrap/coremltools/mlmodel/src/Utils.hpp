#ifndef MLMODEL_UTILS
#define MLMODEL_UTILS

#include <fstream>
#include <memory>
#include <string>
#include <sstream>
#include <limits>
#include <unordered_map>
#include <vector>
#include <functional>

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

        // Support models up to 2GB
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

    /**
     * If a model spec does not use features from later specification versions, this will
     * set the spec version so that the model can be executed on older versions of
     * Core ML. It applies recursively to sub models
     */
    void downgradeSpecificationVersion(Specification::Model *pModel);

    // Helper functions for determining model version
    bool hasCustomLayer(const Specification::Model& model);

    bool hasFlexibleShapes(const Specification::Model& model);
    bool hasIOS11_2Features(const Specification::Model& model);
    bool hasIOS12Features(const Specification::Model& model);

    typedef std::pair<std::string,std::string> StringPair;
    // Returns a vector of pairs of strings, one pair per custom layer instance
    std::vector<StringPair> getCustomLayerNamesAndDescriptions(const Specification::Model& model);
    // Returns a vector of pairs of strings, one pair per custom model instance
    std::vector<StringPair> getCustomModelNamesAndDescriptions(const Specification::Model& model);

    bool hasfp16Weights(const Specification::Model& model);
    bool hasUnsignedQuantizedWeights(const Specification::Model& model);
    bool hasWeightOfType(const Specification::Model& model, const WeightParamType& type);
    bool hasWeightOfType(const Specification::NeuralNetworkLayer& layer, const WeightParamType& type);

    bool hasCustomModel(const Specification::Model& model);
    bool hasAppleWordTagger(const Specification::Model& model);
    bool hasAppleTextClassifier(const Specification::Model& model);
    bool hasAppleImageFeatureExtractor(const Specification::Model& model);
    bool hasCategoricalSequences(const Specification::Model& model);
    bool hasNonmaxSuppression(const Specification::Model& model);
    bool hasBayesianProbitRegressor(const Specification::Model& model);
    bool hasIOS12NewNeuralNetworkLayers(const Specification::Model& model);

    bool hasModelOrSubModelProperty(const Specification::Model& model, const std::function<bool(const Specification::Model&)> &boolFunc);

    // We also need a hasNonmaxSupression and hasBayesianProbitRegressor
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
