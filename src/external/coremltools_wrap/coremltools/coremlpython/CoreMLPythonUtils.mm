#import "CoreMLPythonArray.h"
#import "CoreMLPythonUtils.h"

#include <pybind11/eval.h>
#include <pybind11/numpy.h>

#if PY_MAJOR_VERSION < 3

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#define PyBytes_Check(name) PyString_Check(name)
#pragma clang diagnostic pop
#define PyAnyInteger_Check(name) (PyLong_Check(name) || PyInt_Check(name))

#else

#include <numpy/arrayobject.h>
#define PyAnyInteger_Check(name) (PyLong_Check(name) || (_import_array(), PyArray_IsScalar(name, Integer)))

#endif

using namespace CoreML::Python;

NSURL * Utils::stringToNSURL(const std::string& str) {
    NSString *nsstr = [NSString stringWithUTF8String:str.c_str()];
    return [NSURL fileURLWithPath:nsstr];
}

void Utils::handleError(NSError *error) {
    if (error != nil) {
        NSString *formatted = [NSString stringWithFormat:@"%@", [error userInfo]];
        throw std::runtime_error([formatted UTF8String]);
    }
}

MLDictionaryFeatureProvider * Utils::dictToFeatures(const py::dict& dict, NSError **error) {
    @autoreleasepool {
        NSMutableDictionary<NSString *, NSObject *> *inputDict = [[NSMutableDictionary<NSString *, NSObject *> alloc] init];

        for (const auto element : dict) {
            std::string key = element.first.cast<std::string>();
            NSString *nsKey = [NSString stringWithUTF8String:key.c_str()];
            id nsValue = Utils::convertValueToObjC(element.second);
            inputDict[nsKey] = nsValue;
        }

        return [[MLDictionaryFeatureProvider alloc] initWithDictionary:inputDict error:error];
    }
}

py::dict Utils::featuresToDict(id<MLFeatureProvider> features) {
    @autoreleasepool {
        py::dict ret;
        NSSet<NSString *> *keys = [features featureNames];
        for (NSString *key in keys) {
            MLFeatureValue *value = [features featureValueForName:key];
            py::str pyKey = py::str([key UTF8String]);
            py::object pyValue = convertValueToPython(value);
            ret[pyKey] = pyValue;
        }
        return ret;
    }
}

template<typename KEYTYPE>
static NSObject * convertDictKey(const KEYTYPE& k);

template<>
NSObject * convertDictKey(const int64_t& k) {
    return [NSNumber numberWithLongLong:k];
}

template<>
NSObject * convertDictKey(const std::string& k) {
    return [NSString stringWithUTF8String:k.c_str()];
}

template<typename VALUETYPE>
static NSNumber * convertDictValue(const VALUETYPE& v);

template<>
NSNumber * convertDictValue(const int64_t& v) {
    return [NSNumber numberWithLongLong:v];
}

template<>
NSNumber * convertDictValue(const double& v) {
    return [NSNumber numberWithDouble:v];
}

template<typename KEYTYPE, typename VALUETYPE>
static MLFeatureValue * convertToNSDictionary(const std::unordered_map<KEYTYPE, VALUETYPE>& dict) {
    NSMutableDictionary<NSObject *, NSNumber *> *nsDict = [[NSMutableDictionary<NSObject *, NSNumber *> alloc] init];
    for (const auto& pair : dict) {
        NSObject *key = convertDictKey(pair.first);
        NSNumber *value = convertDictValue(pair.second);
        assert(key != nil);
        nsDict[static_cast<id<NSCopying> _Nonnull>(key)] = value;
    }
    NSError *error = nil;
    MLFeatureValue * ret = [MLFeatureValue featureValueWithDictionary:nsDict error:&error];
    if (error != nil) {
        throw std::runtime_error(error.localizedDescription.UTF8String);
    }
    return ret;
}

static MLFeatureValue * convertValueToDictionary(const py::handle& handle) {

    if(!PyDict_Check(handle.ptr())) {
        throw std::runtime_error("Not a dictionary.");
    }

    // Get the first value in the dictionary; use that as a hint.
    PyObject *key = nullptr, *value = nullptr;
    Py_ssize_t pos = 0;

    int has_values = PyDict_Next(handle.ptr(), &pos, &key, &value);

    // Is it an empty dict?  If so, just return an empty dictionary.
    if(!has_values) {
        return [MLFeatureValue featureValueWithDictionary:@{} error:nullptr];
    }

    if(PyAnyInteger_Check(key)) {
        if(PyAnyInteger_Check(value)) {
            auto dict = handle.cast<std::unordered_map<int64_t, int64_t> >();
            return convertToNSDictionary(dict);
        } else if(PyFloat_Check(value)) {
            auto dict = handle.cast<std::unordered_map<int64_t, double> >();
            return convertToNSDictionary(dict);
        } else {
            throw std::runtime_error("Unknown value type for int key in dictionary.");
        }
    } else if (PyBytes_Check(key) || PyUnicode_Check(key)) {
        if(PyAnyInteger_Check(value)) {
            auto dict = handle.cast<std::unordered_map<std::string, int64_t> >();
            return convertToNSDictionary(dict);
        } else if(PyFloat_Check(value)) {
            auto dict = handle.cast<std::unordered_map<std::string, double> >();
            return convertToNSDictionary(dict);
        } else {
            throw std::runtime_error("Invalid value type for string key in dictionary.");
        }
    } else {
        throw std::runtime_error("Invalid key type dictionary.");
    }
}

static MLFeatureValue * convertValueToArray(const py::handle& handle) {
    // if this line throws, it can't be an array (caller should catch)
    py::array buf = handle.cast<py::array>();
    if (buf.shape() == nullptr) {
        throw std::runtime_error("no shape, can't be an array");
    }
    PybindCompatibleArray *array = [[PybindCompatibleArray alloc] initWithArray:buf];
    return [MLFeatureValue featureValueWithMultiArray:array];
}

static void handleCVReturn(CVReturn status) {
    if (status != kCVReturnSuccess) {
        std::stringstream msg;
        msg << "Got unexpected return code " << status << " from CoreVideo.";
        throw std::runtime_error(msg.str());
    }
}

static MLFeatureValue * convertValueToImage(const py::handle& handle) {
    // assumes handle is a valid PIL image!
    CVPixelBufferRef pixelBuffer = nil;

    size_t width = handle.attr("width").cast<size_t>();
    size_t height = handle.attr("height").cast<size_t>();
    OSType format;
    std::string formatStr = handle.attr("mode").cast<std::string>();

    if (formatStr == "RGB") {
        format = kCVPixelFormatType_32BGRA;
    } else if (formatStr == "RGBA") {
        format = kCVPixelFormatType_32BGRA;
    } else if (formatStr == "L") {
        format = kCVPixelFormatType_OneComponent8;
    } else {
        std::stringstream msg;
        msg << "Unsupported image type " << formatStr << ". ";
        msg << "Supported types are: RGB, RGBA, L.";
        throw std::runtime_error(msg.str());
    }

    CVReturn status = CVPixelBufferCreate(kCFAllocatorDefault, width, height, format, NULL, &pixelBuffer);
    handleCVReturn(status);

    // get bytes out of the PIL image
    py::object tobytes = handle.attr("tobytes");
    py::object bytesResult = tobytes();
    assert(PyBytes_Check(bytesResult.ptr()));
    Py_ssize_t bytesLength = PyBytes_Size(bytesResult.ptr());
    assert(bytesLength >= 0);
    const char *bytesPtr = PyBytes_AsString(bytesResult.ptr());
    std::string bytes(bytesPtr, static_cast<size_t>(bytesLength));

    // copy data into the CVPixelBuffer
    status = CVPixelBufferLockBaseAddress(pixelBuffer, 0);
    handleCVReturn(status);
    void *baseAddress = CVPixelBufferGetBaseAddress(pixelBuffer);
    assert(baseAddress != nullptr);
    assert(!CVPixelBufferIsPlanar(pixelBuffer));
    size_t bytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);
    const char *srcPointer = bytes.data();

    if (formatStr == "RGB") {

        // convert RGB to BGRA
        assert(bytes.size() == width * height * 3);
        for (size_t row = 0; row < height; row++) {
            char *dstPointer = static_cast<char *>(baseAddress) + (row * bytesPerRow);

            for (size_t col = 0; col < width; col++) {

                char R = *srcPointer++;
                char G = *srcPointer++;
                char B = *srcPointer++;

                *dstPointer++ = B;
                *dstPointer++ = G;
                *dstPointer++ = R;
                *dstPointer++ = 0; // A

            }
            assert(bytesPerRow >= width * 4);
        }
        assert(srcPointer == bytes.data() + bytes.size());

    } else if (formatStr == "RGBA") {

        // convert RGBA to BGRA
        assert(bytes.size() == width * height * 4);
        for (size_t row = 0; row < height; row++) {
            char *dstPointer = static_cast<char *>(baseAddress) + (row * bytesPerRow);

            for (size_t col = 0; col < width; col++) {

                char R = *srcPointer++;
                char G = *srcPointer++;
                char B = *srcPointer++;
                char A = *srcPointer++;

                *dstPointer++ = B;
                *dstPointer++ = G;
                *dstPointer++ = R;
                *dstPointer++ = A;

            }
            assert(bytesPerRow >= width * 4);
        }
        assert(srcPointer == bytes.data() + bytes.size());

    } else {

        // assume 8 bit grayscale (the only other case)
        assert(formatStr == "L");
        assert(bytes.size() == width * height);

        for (size_t row = 0; row < height; row++) {
            char *dstPointer = static_cast<char *>(baseAddress) + (row * bytesPerRow);

            std::memcpy(dstPointer, srcPointer, width);
            srcPointer += width;
        }
    }

    assert(srcPointer == bytes.data() + bytes.size());

#ifdef COREML_SHOW_PIL_IMAGES
    if (formatStr == "RGB") {
        // for debugging purposes, convert back to PIL image and show it
        py::object scope = py::module::import("__main__").attr("__dict__");
        py::eval<py::eval_single_statement>("import PIL.Image", scope);
        py::object pilImage = py::eval<py::eval_expr>("PIL.Image");

        std::string cvPixelStr(count, 0);
        const char *basePtr = static_cast<char *>(baseAddress);
        for (size_t row = 0; row < height; row++) {
            for (size_t col = 0; col < width; col++) {
                for (size_t color = 0; color < 3; color++) {
                    cvPixelStr[(row * width * 3) + (col*3) + color] = basePtr[(row * bytesPerRow) + (col*4) + color + 1];
                }
            }
        }

        py::bytes cvPixelBytes = py::bytes(cvPixelStr);
        py::object frombytes = pilImage.attr("frombytes");
        py::str mode = "RGB";
        auto size = py::make_tuple(width, height);
        py::object img = frombytes(mode, size, cvPixelBytes);
        img.attr("show")();
    }
#endif

    status = CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
    handleCVReturn(status);

    return [MLFeatureValue featureValueWithPixelBuffer:pixelBuffer];
}

static bool IsPILImage(const py::handle& handle) {
    // TODO put try/catch around this?

    try {
        py::module::import("PIL.Image");
    } catch(...) {
        return false;
    }

    py::object scope = py::module::import("__main__").attr("__dict__");
    py::eval<py::eval_single_statement>("import PIL.Image", scope);
    py::handle imageTypeHandle = py::eval<py::eval_expr>("PIL.Image.Image", scope);
    assert(PyType_Check(imageTypeHandle.ptr())); // should be a Python type

    return PyObject_TypeCheck(handle.ptr(), (PyTypeObject *)(imageTypeHandle.ptr()));
}

MLFeatureValue * Utils::convertValueToObjC(const py::handle& handle) {

    if (PyAnyInteger_Check(handle.ptr())) {
        try {
            int64_t val = handle.cast<int64_t>();
            return [MLFeatureValue featureValueWithInt64:val];
        } catch(...) {}
    }

    if (PyFloat_Check(handle.ptr())) {
        try {
            double val = handle.cast<double>();
            return [MLFeatureValue featureValueWithDouble:val];
        } catch(...) {}
    }

    if (PyBytes_Check(handle.ptr()) || PyUnicode_Check(handle.ptr())) {
        try {
            std::string val = handle.cast<std::string>();
            return [MLFeatureValue featureValueWithString:[NSString stringWithUTF8String:val.c_str()]];
        } catch(...) {}
    }

    if (PyDict_Check(handle.ptr())) {
        try {
            return convertValueToDictionary(handle);
        } catch(...) {}
    }

    if(PyList_Check(handle.ptr()) || PyTuple_Check(handle.ptr())
       || PyObject_CheckBuffer(handle.ptr())) {
        try {
            return convertValueToArray(handle);
        } catch(...) {}
    }

    if (IsPILImage(handle)) {
        return convertValueToImage(handle);
    }

    py::print("Error: value type not convertible:");
    py::print(handle);
    throw std::runtime_error("value type not convertible");
}

std::vector<size_t> Utils::convertNSArrayToCpp(NSArray<NSNumber *> *array) {
    std::vector<size_t> ret;
    for (NSNumber *value in array) {
        ret.push_back(value.unsignedLongValue);
    }
    return ret;
}

NSArray<NSNumber *>* Utils::convertCppArrayToObjC(const std::vector<size_t>& array) {
    NSMutableArray<NSNumber *>* ret = [[NSMutableArray<NSNumber *> alloc] init];
    for (size_t element : array) {
        [ret addObject:[NSNumber numberWithUnsignedLongLong:element]];
    }
    return ret;
}

static size_t sizeOfArrayElement(MLMultiArrayDataType type) {
    switch (type) {
        case MLMultiArrayDataTypeInt32:
            return sizeof(int32_t);
        case MLMultiArrayDataTypeFloat32:
            return sizeof(float);
        case MLMultiArrayDataTypeDouble:
            return sizeof(double);
        default:
            assert(false);
            return sizeof(double);
    }
}

py::object Utils::convertArrayValueToPython(MLMultiArray *value) {
    if (value == nil) {
        return py::none();
    }
    MLMultiArrayDataType type = value.dataType;
    std::vector<size_t> shape = Utils::convertNSArrayToCpp(value.shape);
    std::vector<size_t> strides = Utils::convertNSArrayToCpp(value.strides);

    // convert strides to numpy (bytes) instead of mlkit (elements)
    for (size_t& stride : strides) {
        stride *= sizeOfArrayElement(type);
    }

    switch (type) {
        case MLMultiArrayDataTypeInt32:
            return py::array(shape, strides, static_cast<int32_t*>(value.dataPointer));
        case MLMultiArrayDataTypeFloat32:
            return py::array(shape, strides, static_cast<float*>(value.dataPointer));
        case MLMultiArrayDataTypeDouble:
            return py::array(shape, strides, static_cast<double*>(value.dataPointer));
        default:
            assert(false);
            return py::object();
    }
}

py::object Utils::convertDictionaryValueToPython(NSDictionary<NSObject *,NSNumber *> * dict) {
    if (dict == nil) {
        return py::none();
    }
    py::dict ret;
    for (NSObject *key in dict) {
        py::object pykey;
        if ([key isKindOfClass:[NSNumber class]]) {
            // can assume int32_t -- we only allow arrays of int or string keys
            NSNumber *nskey = static_cast<NSNumber *>(key);
            pykey = py::int_([nskey integerValue]);
        } else {
            assert([key isKindOfClass:[NSString class]]);
            NSString *nskey = static_cast<NSString *>(key);
            pykey = py::str([nskey UTF8String]);
        }

        NSNumber *value = dict[key];
        ret[pykey] = py::float_([value doubleValue]);
    }
    return ret;
}

py::object Utils::convertImageValueToPython(CVPixelBufferRef value) {
    if (CVPixelBufferIsPlanar(value)) {
        throw std::runtime_error("Only non-planar CVPixelBuffers are currently supported by this Python binding.");
    }

    // supports grayscale and BGRA format types
    auto formatType = CVPixelBufferGetPixelFormatType(value);
    assert(formatType == kCVPixelFormatType_32BGRA || formatType == kCVPixelFormatType_OneComponent8);

    auto result = CVPixelBufferLockBaseAddress(value, kCVPixelBufferLock_ReadOnly);
    assert(result == kCVReturnSuccess);

    uint8_t *src = reinterpret_cast<uint8_t*>(CVPixelBufferGetBaseAddress(value));
    assert(src != nullptr);

    auto height = CVPixelBufferGetHeight(value);
    auto width = CVPixelBufferGetWidth(value);
    size_t srcBytesPerRow = CVPixelBufferGetBytesPerRow(value);
    // Initializing this for Xcode warnings
    size_t dstBytesPerRow = 0;
    py::str mode;
    if (formatType == kCVPixelFormatType_32BGRA) {
        dstBytesPerRow = width * 4;
        mode = "RGBA";
    } else if (formatType == kCVPixelFormatType_OneComponent8) {
        dstBytesPerRow = width;
        mode = "L";
    }
    std::string array(height * dstBytesPerRow, 0);
    for (size_t i=0; i<height; i++) {
        for (size_t j=0; j<width; j++) {
            if (formatType == kCVPixelFormatType_32BGRA) {
                // convert BGRA to RGBA
                array[(i * dstBytesPerRow) + (j * 4) + 0] = static_cast<char>(src[(i * srcBytesPerRow) + (j * 4) + 2]);
                array[(i * dstBytesPerRow) + (j * 4) + 1] = static_cast<char>(src[(i * srcBytesPerRow) + (j * 4) + 1]);
                array[(i * dstBytesPerRow) + (j * 4) + 2] = static_cast<char>(src[(i * srcBytesPerRow) + (j * 4) + 0]);
                array[(i * dstBytesPerRow) + (j * 4) + 3] = static_cast<char>(src[(i * srcBytesPerRow) + (j * 4) + 3]);
            } else if (formatType == kCVPixelFormatType_OneComponent8) {
                array[(i * dstBytesPerRow) + j] = static_cast<char>(src[(i * srcBytesPerRow) + j]);
            }
        }
    }

    result = CVPixelBufferUnlockBaseAddress(value, kCVPixelBufferLock_ReadOnly);
    assert(result == kCVReturnSuccess);

    py::object scope = py::module::import("__main__").attr("__dict__");
    py::eval<py::eval_single_statement>("import PIL.Image", scope);
    py::object pilImage = py::eval<py::eval_expr>("PIL.Image", scope);
    py::object frombytes = pilImage.attr("frombytes");
    py::object img = frombytes(mode, py::make_tuple(width, height), py::bytes(array));
    return img;
}

py::object Utils::convertValueToPython(MLFeatureValue *value) {
    switch ([value type]) {
        case MLFeatureTypeInt64:
            return py::int_(value.int64Value);
        case MLFeatureTypeMultiArray:
            return convertArrayValueToPython(value.multiArrayValue);
        case MLFeatureTypeImage:
            return convertImageValueToPython(value.imageBufferValue);
        case MLFeatureTypeDouble:
            return py::float_(value.doubleValue);
        case MLFeatureTypeString:
            return py::str(value.stringValue.UTF8String);
        case MLFeatureTypeDictionary:
            return convertDictionaryValueToPython(value.dictionaryValue);
        case MLFeatureTypeSequence:
            // rdar://problem/38885937
            throw std::runtime_error("convertValueToPython not implemented for MLFeatureTypeSequence");
            return py::none();
        case MLFeatureTypeInvalid:
            assert(false);
            return py::none();
    }
    return py::object();
}



py::dict Utils::shapeConstraintToPyDict(const ShapeConstraint& constraint) {
    @autoreleasepool {
        py::dict ret;
        ret[py::str("S")] = py::make_tuple((int)constraint.sequenceRange().minimumValue(), (constraint.sequenceRange().maximumValue().isUnbound() ? -1 : (int)constraint.sequenceRange().maximumValue().value()));
        ret[py::str("B")] = py::make_tuple((int)constraint.batchRange().minimumValue(), (constraint.batchRange().maximumValue().isUnbound() ? -1 : (int)constraint.batchRange().maximumValue().value()));
        ret[py::str("C")] = py::make_tuple((int)constraint.channelRange().minimumValue(), (constraint.channelRange().maximumValue().isUnbound() ? -1 : (int)constraint.channelRange().maximumValue().value()));
        ret[py::str("H")] = py::make_tuple((int)constraint.heightRange().minimumValue(), (constraint.heightRange().maximumValue().isUnbound() ? -1 : (int)constraint.heightRange().maximumValue().value()));
        ret[py::str("W")] = py::make_tuple((int)constraint.widthRange().minimumValue(), (constraint.widthRange().maximumValue().isUnbound() ? -1 : (int)constraint.widthRange().maximumValue().value()));
        return ret;
    }
}
