#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wdocumentation"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#pragma clang diagnostic pop

#import <CoreML/CoreML.h>
#import "NeuralNetworkShapes.hpp"

namespace py = pybind11;

namespace CoreML {
    namespace Python {

        class Model {
        private:
            MLModel *m_model = nil;
            NSURL *compiledUrl = nil;
        public:
            Model(const Model&) = delete;
            Model& operator=(const Model&) = delete;
            ~Model();
            explicit Model(const std::string& urlStr);
            py::dict predict(const py::dict& input, bool useCPUOnly);
            static int32_t maximumSupportedSpecificationVersion();
            std::string toString() const;
        };


        class NeuralNetworkShapeInformation {
        private:
            std::unique_ptr<NeuralNetworkShaper> shaper;
        public:
            NeuralNetworkShapeInformation(const std::string& filename);
            NeuralNetworkShapeInformation(const std::string& filename, bool useInputAndOutputConstraints);
            void init(const std::string& filename);
            py::dict shape(const std::string& name);
            std::string toString() const;
            void print() const;
        };


    }
}
