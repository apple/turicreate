#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#pragma clang diagnostic pop

#import <CoreML/CoreML.h>
#import "NeuralNetworkBuffer.hpp"
#import "Validation/NeuralNetwork/NeuralNetworkShapes.hpp"

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
            explicit Model(const std::string& urlStr, bool useCPUOnly);
            py::dict predict(const py::dict& input, bool useCPUOnly);
            static py::bytes autoSetSpecificationVersion(const py::bytes& modelBytes);
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

        // TODO:
        // Create template class and create instance with respect
        // to datatypes
        class NeuralNetworkBufferInformation {
            private:
                std::unique_ptr<NNBuffer::NeuralNetworkBuffer> nnBuffer;

            public:
                NeuralNetworkBufferInformation(const std::string& bufferFilePath, NNBuffer::bufferMode mode);
                ~NeuralNetworkBufferInformation();
                std::vector<float> getBuffer(const u_int64_t offset);
                u_int64_t addBuffer(const std::vector<float>& buffer);
        };
    }
}
