#import <CoreML/CoreML.h>
#import "CoreMLPythonArray.h"
#import "CoreMLPython.h"
#import "CoreMLPythonUtils.h"
#import "Globals.hpp"
#import "Utils.hpp"
#import <fstream>
#import <vector>
#import "NeuralNetworkBuffer.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-prototypes"

#if ! __has_feature(objc_arc)
#error "ARC is off"
#endif

namespace py = pybind11;

using namespace CoreML::Python;

Model::~Model() {
    @autoreleasepool {
        NSFileManager *fileManager = [NSFileManager defaultManager];
        if (compiledUrl != nil) {
            [fileManager removeItemAtURL:compiledUrl error:NULL];
        }
    }
}

Model::Model(const std::string& urlStr, bool useCPUOnly) {
    @autoreleasepool {

        // Compile the model
        NSError *error = nil;
        NSURL *specUrl = Utils::stringToNSURL(urlStr);

        // Swallow output for the very verbose coremlcompiler
        int stdoutBack = dup(STDOUT_FILENO);
        int devnull = open("/dev/null", O_WRONLY);
        dup2(devnull, STDOUT_FILENO);

        // Compile the model
        compiledUrl = [MLModel compileModelAtURL:specUrl error:&error];

        // Close all the file descriptors and revert back to normal
        dup2(stdoutBack, STDOUT_FILENO);
        close(devnull);
        close(stdoutBack);

        // Translate into a type that pybind11 can bridge to Python
        if (error != nil) {
            std::stringstream errmsg;
            errmsg << "Error compiling model: \"";
            errmsg << error.localizedDescription.UTF8String;
            errmsg << "\".";
            throw std::runtime_error(errmsg.str());
        }

        if (@available(macOS 10.14, *)) {
            MLModelConfiguration *configuration = [MLModelConfiguration new];
            if (useCPUOnly){
                configuration.computeUnits = MLComputeUnitsCPUOnly;
            }
            m_model = [MLModel modelWithContentsOfURL:compiledUrl configuration:configuration error:&error];
        } else {
            m_model = [MLModel modelWithContentsOfURL:compiledUrl error:&error];
        }
        Utils::handleError(error);
    }
}

py::dict Model::predict(const py::dict& input, bool useCPUOnly) {
    @autoreleasepool {
        NSError *error = nil;
        MLDictionaryFeatureProvider *inFeatures = Utils::dictToFeatures(input, &error);
        Utils::handleError(error);
        MLPredictionOptions *options = [[MLPredictionOptions alloc] init];
        options.usesCPUOnly = useCPUOnly;
        id<MLFeatureProvider> outFeatures = [m_model predictionFromFeatures:static_cast<MLDictionaryFeatureProvider * _Nonnull>(inFeatures)
                                                                    options:options
                                                                      error:&error];
        Utils::handleError(error);
        return Utils::featuresToDict(outFeatures);
    }
}

py::bytes Model::autoSetSpecificationVersion(const py::bytes& modelBytes) {

    CoreML::Specification::Model model;
    std::istringstream modelIn(static_cast<std::string>(modelBytes), std::ios::binary);
    CoreML::loadSpecification<Specification::Model>(model, modelIn);
    model.set_specificationversion(CoreML::MLMODEL_SPECIFICATION_VERSION_NEWEST);
    // always try to downgrade the specification version to the
    // minimal version that supports everything in this mlmodel
    CoreML::downgradeSpecificationVersion(&model);
    std::ostringstream modelOut;
    saveSpecification(model, modelOut);
    return static_cast<py::bytes>(modelOut.str());

}

int32_t Model::maximumSupportedSpecificationVersion() {
    return CoreML::MLMODEL_SPECIFICATION_VERSION_NEWEST;
}

NeuralNetworkShapeInformation::NeuralNetworkShapeInformation(const std::string& filename) {
    CoreML::Specification::Model model;
    Result r = CoreML::loadSpecificationPath(model, filename);
    shaper = std::unique_ptr<NeuralNetworkShaper>(new NeuralNetworkShaper(model));
}

NeuralNetworkShapeInformation::NeuralNetworkShapeInformation(const std::string& filename, bool useInputAndOutputConstraints) {
    CoreML::Specification::Model model;
    Result r = CoreML::loadSpecificationPath(model, filename);
    shaper = std::unique_ptr<NeuralNetworkShaper>(new NeuralNetworkShaper(model, useInputAndOutputConstraints));
}

void NeuralNetworkShapeInformation::init(const std::string& filename) {
    CoreML::Specification::Model model;
    Result r = CoreML::loadSpecificationPath(model, filename);
    shaper.reset(new NeuralNetworkShaper(model));
}

py::dict NeuralNetworkShapeInformation::shape(const std::string& name) {
    const ShapeConstraint& constraint = shaper->shape(name);
    return Utils::shapeConstraintToPyDict(constraint);
}

void NeuralNetworkShapeInformation::print() const {
    shaper->print();
}

/*
 * NeuralNetworkBuffer - NeuralNetworkBuffer
 */
NeuralNetworkBufferInformation::NeuralNetworkBufferInformation(const std::string &bufferFilePath, NNBuffer::bufferMode mode)
    : nnBuffer(std::make_unique<NNBuffer::NeuralNetworkBuffer>(bufferFilePath, mode))
{
}

/*
 * NeuralNetworkBufferInformation - ~NeuralNetworkBufferInformation
 */
NeuralNetworkBufferInformation::~NeuralNetworkBufferInformation() = default;

/*
 * NeuralNetworkBuffer - addBuffer
 * Writes given buffer into file
 * Returns offset from the beginning of buffer
 */
inline u_int64_t NeuralNetworkBufferInformation::addBuffer(const std::vector<float> &buffer) {
    return nnBuffer->addBuffer(buffer);
}

/*
 * NeuralNetworkBufferInformation - getBuffer
 * Reads buffer from given offset and of given size and writes to data
 */
inline std::vector<float> NeuralNetworkBufferInformation::getBuffer(const u_int64_t offset) {
    // TODO: Explore Pybind11 Opaque to pass vector by reference
    std::vector<float> buffer;
    nnBuffer->getBuffer(offset, buffer);
    return buffer;
}

PYBIND11_PLUGIN(libcoremlpython) {
    py::module m("libcoremlpython", "CoreML.Framework Python bindings");

    py::class_<Model>(m, "_MLModelProxy")
        .def(py::init<const std::string&, bool>())
        .def("predict", &Model::predict)
        .def_static("auto_set_specification_version", &Model::autoSetSpecificationVersion)
        .def_static("maximum_supported_specification_version", &Model::maximumSupportedSpecificationVersion);

    py::class_<NeuralNetworkShapeInformation>(m, "_NeuralNetworkShaperProxy")
        .def(py::init<const std::string&>())
        .def(py::init<const std::string&, bool>())
        .def("shape", &NeuralNetworkShapeInformation::shape)
        .def("print", &NeuralNetworkShapeInformation::print);

    py::class_<NeuralNetworkBufferInformation> netBuffer(m, "_NeuralNetworkBuffer");
    netBuffer.def(py::init<const std::string&, NNBuffer::bufferMode>())
        .def("add_buffer", &NeuralNetworkBufferInformation::addBuffer)
        .def("get_buffer", &NeuralNetworkBufferInformation::getBuffer);
    py::enum_<NNBuffer::bufferMode>(netBuffer, "mode")
        .value("write", NNBuffer::bufferMode::write)
        .value("append", NNBuffer::bufferMode::append)
        .value("read", NNBuffer::bufferMode::read)
        .export_values();

    return m.ptr();
}

#pragma clang diagnostic pop
