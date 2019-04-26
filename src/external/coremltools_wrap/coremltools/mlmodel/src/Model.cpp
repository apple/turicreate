#include "Comparison.hpp"
#include "Format.hpp"
#include "Model.hpp"
#include "Utils.hpp"

#include <fstream>
#include <unordered_map>

namespace CoreML {

    Model::Model() {
        m_spec = std::make_shared<Specification::Model>();
        m_spec->set_specificationversion(MLMODEL_SPECIFICATION_VERSION);
    }

    Model::Model(const Specification::Model& proto) {
        m_spec = std::make_shared<Specification::Model>(proto);
        // We need to check this here because the proto could be overly strict
        downgradeSpecificationVersion();
    }

    Model::Model(const std::string& description)
    : Model::Model() {
        Specification::Metadata* metadata = m_spec->mutable_description()->mutable_metadata();
        metadata->set_shortdescription(description);
    }

    Model::Model(const Model& other) = default;
    Model::~Model() = default;

    Result Model::validateGeneric(const Specification::Model& model) {
        // make sure compat version fields are filled in
        if (model.specificationversion() == 0) {
            return Result(ResultType::INVALID_COMPATIBILITY_VERSION,
                          "Model specification version field missing or corrupt.");
        }

        // check public compatibility version
        // note: this one should always be backward compatible, so use > here
        if (model.specificationversion() > MLMODEL_SPECIFICATION_VERSION) {
            std::stringstream msg;
            msg << "The .mlmodel supplied is of version "
                << model.specificationversion()
                << ", intended for a newer version of Xcode. This version of Xcode supports model version "
                << MLMODEL_SPECIFICATION_VERSION
                << " or earlier.";
            return Result(ResultType::UNSUPPORTED_COMPATIBILITY_VERSION,
                          msg.str());
        }

        // validate model interface
        Result r = validateModelDescription(model.description(), model.specificationversion());
        if (!r.good()) {
            return r;
        }

        return validateOptional(model);
    }

#define VALIDATE_MODEL_TYPE(TYPE) \
    case MLModelType_ ## TYPE: \
        return ::CoreML::validate<MLModelType_ ## TYPE>(model);

    Result Model::validate(const Specification::Model& model) {
        Result result = validateGeneric(model);
        if (!result.good()) { return result; }

        MLModelType type = static_cast<MLModelType>(model.Type_case());
        // TODO -- is there a better way to do this than switch/case?
        // the compiler doesn't like <type> being unknown at compile time.
        switch (type) {
                VALIDATE_MODEL_TYPE(pipelineClassifier);
                VALIDATE_MODEL_TYPE(pipelineRegressor);
                VALIDATE_MODEL_TYPE(pipeline);
                VALIDATE_MODEL_TYPE(glmClassifier);
                VALIDATE_MODEL_TYPE(glmRegressor);
                VALIDATE_MODEL_TYPE(treeEnsembleClassifier);
                VALIDATE_MODEL_TYPE(treeEnsembleRegressor);
                VALIDATE_MODEL_TYPE(supportVectorClassifier);
                VALIDATE_MODEL_TYPE(supportVectorRegressor);
                VALIDATE_MODEL_TYPE(neuralNetworkClassifier);
                VALIDATE_MODEL_TYPE(neuralNetworkRegressor);
                VALIDATE_MODEL_TYPE(neuralNetwork);
                VALIDATE_MODEL_TYPE(oneHotEncoder);
                VALIDATE_MODEL_TYPE(arrayFeatureExtractor);
                VALIDATE_MODEL_TYPE(featureVectorizer);
                VALIDATE_MODEL_TYPE(imputer);
                VALIDATE_MODEL_TYPE(dictVectorizer);
                VALIDATE_MODEL_TYPE(scaler);
                VALIDATE_MODEL_TYPE(nonMaximumSuppression);
                VALIDATE_MODEL_TYPE(categoricalMapping);
                VALIDATE_MODEL_TYPE(normalizer);
                VALIDATE_MODEL_TYPE(identity);
                VALIDATE_MODEL_TYPE(customModel);
                VALIDATE_MODEL_TYPE(bayesianProbitRegressor);
                VALIDATE_MODEL_TYPE(wordTagger);
                VALIDATE_MODEL_TYPE(textClassifier);
                VALIDATE_MODEL_TYPE(visionFeaturePrint);
            case MLModelType_NOT_SET:
                return Result(ResultType::INVALID_MODEL_INTERFACE, "Model did not specify a valid model-parameter type.");
        }
    }

    Result Model::validate() const {
        return Model::validate(*m_spec);
    }

    Result Model::load(std::istream& in, Model& out) {
        if (!in.good()) {
            return Result(ResultType::UNABLE_TO_OPEN_FILE,
                          "unable to open file for read");
        }


        Result r = loadSpecification(*(out.m_spec), in);
        if (!r.good()) { return r; }
        // validate on load
        r = out.validate();

        return r;
    }

    Result Model::load(const std::string& path, Model& out) {
        std::ifstream in(path, std::ios::binary);
        return load(in, out);
    }

    // We will only reduce the given specification version if possible. We never increase it here.
    void Model::downgradeSpecificationVersion() {
        CoreML::downgradeSpecificationVersion(m_spec.get());
    }

    Result Model::save(std::ostream& out) {
        if (!out.good()) {
            return Result(ResultType::UNABLE_TO_OPEN_FILE,
                          "unable to open file for write");
        }

        downgradeSpecificationVersion();

        // validate on save
        Result r = validate();
        if (!r.good()) {
            return r;
        }

        return saveSpecification(*m_spec, out);
    }

    Result Model::save(const std::string& path) {
        std::ofstream out(path, std::ios::binary);
        return save(out);
    }

    const std::string& Model::shortDescription() const {
        return m_spec->description().metadata().shortdescription();
    }

    SchemaType Model::inputSchema() const {
        SchemaType inputs;
        const Specification::ModelDescription& interface = m_spec->description();
        int size = interface.input_size();
        assert(size >= 0);
        inputs.reserve(static_cast<size_t>(size));
        for (int i = 0; i < size; i++) {
            const Specification::FeatureDescription &desc = interface.input(i);
            inputs.push_back(std::make_pair(desc.name(), desc.type()));
        }
        return inputs;
    }

    SchemaType Model::outputSchema() const {
        SchemaType outputs;
        const Specification::ModelDescription& interface = m_spec->description();
        int size = interface.output_size();
        assert(size >= 0);
        outputs.reserve(static_cast<size_t>(size));
        for (int i = 0; i < size; i++) {
            const Specification::FeatureDescription &desc = interface.output(i);
            outputs.push_back(std::make_pair(desc.name(), desc.type()));
        }
        return outputs;
    }

    Result Model::addInput(const std::string& featureName,
                           FeatureType featureType) {
        Specification::ModelDescription* interface = m_spec->mutable_description();
        Specification::FeatureDescription *arg = interface->add_input();
        arg->set_name(featureName);
        arg->set_allocated_type(featureType.allocateCopy());
        return Result();
    }

    Result Model::addOutput(const std::string& targetName,
                            FeatureType targetType) {
        Specification::ModelDescription* interface = m_spec->mutable_description();
        Specification::FeatureDescription *arg = interface->add_output();
        arg->set_name(targetName);
        arg->set_allocated_type(targetType.allocateCopy());
        return Result();
    }

    MLModelType Model::modelType() const {
        return static_cast<MLModelType>(m_spec->Type_case());
    }

    std::string Model::modelTypeName() const {
        return MLModelType_Name(modelType());
    }

    const Specification::Model& Model::getProto() const {
        return *m_spec;
    }

    Specification::Model& Model::getProto() {
        return *m_spec;
    }

    Result Model::enforceTypeInvariant(const std::vector<FeatureType>& allowedFeatureTypes,
                                       FeatureType featureType) {

        for (const FeatureType& t : allowedFeatureTypes) {
            if (featureType == t) {
                // no invariant broken -- type matches one of the allowed types
                return Result();
            }
        }

        return Result::featureTypeInvariantError(allowedFeatureTypes, featureType);
    }

    bool Model::operator==(const Model& other) const {
        return *m_spec == *(other.m_spec);
    }

    bool Model::operator!=(const Model& other) const {
        return !(*this == other);
    }

    static void writeFeatureDescription(std::stringstream& ss,
                                        const Specification::FeatureDescription& feature) {
        ss  << "\t\t"
            << feature.name()
            << " ("
            << FeatureType(feature.type()).toString()
            << ")";
        if (feature.shortdescription() != "") {
            ss << ": " << feature.shortdescription();
        }
        ss << "\n";
    }

    void Model::toStringStream(std::stringstream& ss) const {
        ss << "Spec version: " << m_spec->specificationversion() << "\n";
        ss << "Model type: " << MLModelType_Name(static_cast<MLModelType>(m_spec->Type_case())) << "\n";
        ss << "Interface:" << "\n";
        ss << "\t" << "Inputs:" << "\n";
        for (const auto& input : m_spec->description().input()) {
            writeFeatureDescription(ss, input);
        }
        ss << "\t" << "Outputs:" << "\n";
        for (const auto& output : m_spec->description().output()) {
            writeFeatureDescription(ss, output);
        }
        if (m_spec->description().predictedfeaturename() != "") {
            ss << "\t" << "Predicted feature name: " << m_spec->description().predictedfeaturename() << "\n";
        }
        if (m_spec->description().predictedprobabilitiesname() != "") {
            ss << "\t" << "Predicted probability name: " << m_spec->description().predictedprobabilitiesname() << "\n";
        }
    }

    std::string Model::toString() const {
        std::stringstream ss;
        toStringStream(ss);
        return ss.str();
    }
}

#pragma mark C exports

extern "C" {

_MLModelSpecification::_MLModelSpecification()
    : cppFormat(new CoreML::Specification::Model())
{
}

    _MLModelSpecification::_MLModelSpecification(const CoreML::Specification::Model& te)
: cppFormat(new CoreML::Specification::Model(te))
{
}

    _MLModelSpecification::_MLModelSpecification(const CoreML::Model& te) {
  cppFormat.reset(new CoreML::Specification::Model(te.getProto()));
}

_MLModelMetadataSpecification::_MLModelMetadataSpecification() : cppMetadata(new CoreML::Specification::Metadata())
{
}

_MLModelMetadataSpecification::_MLModelMetadataSpecification(const CoreML::Specification::Metadata& meta)
: cppMetadata(new CoreML::Specification::Metadata(meta))
{
}

_MLModelDescriptionSpecification::_MLModelDescriptionSpecification() : cppInterface(new CoreML::Specification::ModelDescription())
{
}

_MLModelDescriptionSpecification::_MLModelDescriptionSpecification(const CoreML::Specification::ModelDescription& interface)
: cppInterface(new CoreML::Specification::ModelDescription(interface))
{
}

}
