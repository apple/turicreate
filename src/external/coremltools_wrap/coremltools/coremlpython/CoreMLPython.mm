#import <CoreML/CoreML.h>
#import "CoreMLPythonArray.h"
#import "CoreMLPython.h"
#import "CoreMLPythonUtils.h"
#import "Globals.hpp"
#import "NeuralNetworkShapes.hpp"
#import "Utils.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-prototypes"

#if ! __has_feature(objc_arc)
#error "ARC is off"
#endif

namespace py = pybind11;

using namespace CoreML::Python;

Model::~Model() {
    NSError *error = nil;
    NSFileManager *fileManager = [NSFileManager defaultManager];
    if (compiledUrl != nil) {
        [fileManager removeItemAtPath:[[compiledUrl URLByDeletingLastPathComponent] path]  error:&error];
    }
}

Model::Model(const std::string& urlStr) {
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

        m_model = [MLModel modelWithContentsOfURL:compiledUrl error:&error];
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

int32_t Model::maximumSupportedSpecificationVersion() {
    return CoreML::MLMODEL_SPECIFICATION_VERSION;
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

PYBIND11_PLUGIN(libcoremlpython) {
    py::module m("libcoremlpython", "CoreML.Framework Python bindings");

    py::class_<Model>(m, "_MLModelProxy")
        .def(py::init<const std::string&>())
        .def("predict", &Model::predict)
        .def_static("maximum_supported_specification_version", &Model::maximumSupportedSpecificationVersion);

    py::class_<NeuralNetworkShapeInformation>(m, "_NeuralNetworkShaperProxy")
        .def(py::init<const std::string&>())
        .def(py::init<const std::string&, bool>())
        .def("shape", &NeuralNetworkShapeInformation::shape)
        .def("print", &NeuralNetworkShapeInformation::print);

    return m.ptr();
}

#pragma clang diagnostic pop
