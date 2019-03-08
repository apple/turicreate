#ifndef ML_MODEL_HPP
#define ML_MODEL_HPP

#include <memory>
#include <sstream>
#include <string>

#include "Globals.hpp"
#include "Result.hpp"
#include "Validators.hpp"

#include "../build/format/Model_enums.h"
#include "../build/format/Normalizer_enums.h"

namespace CoreML {

namespace Specification {
    class Model;
    class ModelDescription;
    class Metadata;
}

/**
 * The primary interface to the whole model spec .  Provides functionality for
 * saving and loading model specs, validating them, and incrementally building
 * them by adding transforms.
 */
class Model {
private:
    static Result validateGeneric(const Specification::Model& model);

protected:
    std::shared_ptr<Specification::Model> m_spec;

public:
    Model();
    Model(const std::string& description);
    Model(const Specification::Model& proto);
    Model(const Model& other);
    virtual ~Model();

    // operator overloads
    bool operator== (const Model& other) const;
    bool operator!= (const Model& other) const;

    /**
     * Deserializes a MLModel from an std::istream.
     *
     * @param stream the input stream.
     * @param out a MLModel reference that will be overwritten with the loaded
     *        MLModel if successful.
     * @return the result of the load operation, with ResultType::NO_ERROR on success.
     */
    static Result load(std::istream& stream,
                       Model& out);

    /**
     * Deserializes a MLModel from a file given by a path.
     *
     * @param path the input file path.
     * @param out MLModel reference loaded MLModel if successful.
     * @return the result of the load operation, with ResultType::NO_ERROR on
     * success.
     */
    static Result load(const std::string& path,
                       Model& out);

    /**
     * Serializes a MLModel to an std::ostream.
     *
     * @param stream the output stream.
     * @return the result of the save operation, with ResultType::NO_ERROR on success.
     */
    Result save(std::ostream& stream);

    /**
     * Serializes a MLModel to a file given by a path.
     *
     * @param path the output file path.
     * @return result of save operation, with ResultType::NO_ERROR on success.
     */
    Result save(const std::string& path);


    const std::string& shortDescription() const;
    MLModelType modelType() const;
    std::string modelTypeName() const;

    /**
     * Get the schema (name, type) for the inputs of this transform.
     *
     * @return input schema for outputs in this transform.
     */
    SchemaType inputSchema() const;

    /**
     * Get the output schema (name, type) for the inputs of this transform.
     *
     * @return output schema for outputs in this transform.
     */
    SchemaType outputSchema() const;

    /**
     * Enforces type invariant conditions.
     *
     * Enforces type invariant conditions for this TransformSpec, given a list
     * of allowed feature types and a proposed feature type. If the
     * proposed feature type is in the list of allowed feature types, this
     * function returns a Result with type ResultType::NO_ERROR. If the
     * proposed feature type is not in the list of allowed feature types,
     * this function returns a Result with type
     * ResultType::FEATURE_TYPE_INVARIANT_VIOLATION.
     *
     * @param allowedFeatureTypes the list of allowed feature types for
     *        this transform.
     * @param featureType the proposed feature type of a feature for this
     *        transform.
     * @return a Result corresponding to whether featureType is contained
     *         in allowedFeatureTypes.
     */
    static Result enforceTypeInvariant(const std::vector<FeatureType>&
                                       allowedFeatureTypes, FeatureType featureType);


    /**
     * Ensures the spec is valid. This gets called every time before the
     * spec gets added to the MLModel.
     *
     * @return ResultType::NO_ERROR if the transform is valid.
     */
    static Result validate(const Specification::Model& model);
    Result validate() const;

    /**
     * Add an input to the transform-spec.
     *
     * @param featureName Name of the feature to be added.
     * @param featureType Type of the feature added.
     * @return Result type of this operation.
     */
    virtual Result addInput(const std::string& featureName, FeatureType featureType);

    /**
     * Add an output to the transform-spec.
     *
     * @param outputName  Name of the feature to be added.
     * @param outputType Type of the feature added.
     * @return Result type of this operation.
     */
    virtual Result addOutput(const std::string& outputName, FeatureType outputType);

    /**
     * If a model does not use features from later specification versions, this will
     * set the spec version so that the model can be executed on older versions of
     * Core ML.
     */
    void downgradeSpecificationVersion();

    // TODO -- This seems like a giant hack. This is leaking abstractions.
    const Specification::Model& getProto() const;
    Specification::Model& getProto();

    // string representation (text description)
    std::string toString() const;
    void toStringStream(std::stringstream& ss) const;
  };
}

extern "C" {

/**
 * C-Structs needed for integration with pure C.
 */
typedef struct _MLModelSpecification {
    std::shared_ptr<CoreML::Specification::Model> cppFormat;
    _MLModelSpecification();
    _MLModelSpecification(const CoreML::Specification::Model&);
    _MLModelSpecification(const CoreML::Model&);
} MLModelSpecification;

typedef struct _MLModelMetadataSpecification {
    std::shared_ptr<CoreML::Specification::Metadata> cppMetadata;
    _MLModelMetadataSpecification();
    _MLModelMetadataSpecification(const CoreML::Specification::Metadata&);
} MLModelMetadataSpecification;

typedef struct _MLModelDescriptionSpecification {
    std::shared_ptr<CoreML::Specification::ModelDescription> cppInterface;
    _MLModelDescriptionSpecification();
    _MLModelDescriptionSpecification(const CoreML::Specification::ModelDescription&);
} MLModelDescriptionSpecification;

}
#endif
