#include "Comparison.hpp"
#include "DataType.hpp"
#include "Format.hpp"

#include <sstream>

namespace CoreML {

    FeatureType::FeatureType(MLFeatureTypeType type)
    : m_type(std::make_shared<Specification::FeatureType>()) {
        switch (type) {
            case MLFeatureTypeType_NOT_SET:
                break;
            case MLFeatureTypeType_multiArrayType:
                m_type->mutable_multiarraytype();
                break;
            case MLFeatureTypeType_imageType:
                m_type->mutable_imagetype();
                break;
            case MLFeatureTypeType_int64Type:
                m_type->mutable_int64type();
                break;
            case MLFeatureTypeType_doubleType:
                m_type->mutable_doubletype();
                break;
            case MLFeatureTypeType_stringType:
                m_type->mutable_stringtype();
                break;
            case MLFeatureTypeType_dictionaryType:
                m_type->mutable_dictionarytype();
                break;
            case MLFeatureTypeType_sequenceType:
                m_type->mutable_sequencetype();
                break;
        }
    }

    FeatureType::FeatureType(const Specification::FeatureType& wrapped)
    : m_type(std::make_shared<Specification::FeatureType>(wrapped)) {
    }

    // simple types
#define WRAP_SIMPLE_TYPE(T, U) \
FeatureType FeatureType::T() { return FeatureType(U); }

    WRAP_SIMPLE_TYPE(Int64, MLFeatureTypeType_int64Type)
    WRAP_SIMPLE_TYPE(String, MLFeatureTypeType_stringType)
    WRAP_SIMPLE_TYPE(Image, MLFeatureTypeType_imageType) /* TODO image is not simple type */
    WRAP_SIMPLE_TYPE(Double, MLFeatureTypeType_doubleType)

    // parametric types
    FeatureType FeatureType::Array(const std::vector<int64_t> shape, MLArrayDataType dataType) {
        FeatureType out(MLFeatureTypeType_multiArrayType);
        Specification::ArrayFeatureType *params = out->mutable_multiarraytype();

        for (int64_t s : shape) {
            params->add_shape(s);
        }
        params->set_datatype(static_cast<Specification::ArrayFeatureType::ArrayDataType>(dataType));
        return out;
    }

    FeatureType FeatureType::Array(const std::vector<int64_t> shape) {
        return Array(shape,MLArrayDataTypeDOUBLE);
    }

    FeatureType FeatureType::Dictionary(MLDictionaryFeatureTypeKeyType keyType) {
        FeatureType out(MLFeatureTypeType_dictionaryType);

        Specification::DictionaryFeatureType *params = out->mutable_dictionarytype();

        switch (keyType) {
            case MLDictionaryFeatureTypeKeyType_int64KeyType:
                params->mutable_int64keytype();
                break;
            case MLDictionaryFeatureTypeKeyType_stringKeyType:
                params->mutable_stringkeytype();
                break;
            case MLDictionaryFeatureTypeKeyType_NOT_SET:
                throw std::runtime_error("Invalid dictionary key type. Expected one of: {int64, string}.");
        }

        return out;
    }

    // operators
    const Specification::FeatureType& FeatureType::operator*() const {
        return *m_type;
    }

    Specification::FeatureType& FeatureType::operator*() {
        return *m_type;
    }

    const Specification::FeatureType* FeatureType::operator->() const {
        return m_type.get();
    }

    Specification::FeatureType* FeatureType::operator->() {
        return m_type.get();
    }

    bool FeatureType::operator==(const FeatureType& other) const {
        return *m_type == *other.m_type;
    }

    bool FeatureType::operator!=(const FeatureType& other) const {
        return !(*this == other);
    }

    static std::string featureTypeToString(Specification::FeatureType::TypeCase tag) {
        switch (tag) {
            case Specification::FeatureType::kMultiArrayType:
                return "MultiArray";
            case Specification::FeatureType::kDictionaryType:
                return "Dictionary";
            case Specification::FeatureType::kImageType:
                return "Image";
            case Specification::FeatureType::kDoubleType:
                return "Double";
            case Specification::FeatureType::kInt64Type:
                return "Int64";
            case Specification::FeatureType::kStringType:
                return "String";
            case Specification::FeatureType::kSequenceType:
                return "Sequence";
            case Specification::FeatureType::TYPE_NOT_SET:
                return "Invalid";
        }
    }

    static std::string keyTypeToString(Specification::DictionaryFeatureType::KeyTypeCase tag) {
        switch (tag) {
            case Specification::DictionaryFeatureType::kInt64KeyType:
                return "Int64";
            case Specification::DictionaryFeatureType::kStringKeyType:
                return "String";
            case Specification::DictionaryFeatureType::KEYTYPE_NOT_SET:
                return "Invalid";
        }
    }

    static std::string dataTypeToString(Specification::ArrayFeatureType_ArrayDataType dataType) {
        switch (dataType) {
            case Specification::ArrayFeatureType_ArrayDataType_INT32:
                return "Int32";
            case Specification::ArrayFeatureType_ArrayDataType_DOUBLE:
                return "Double";
            case Specification::ArrayFeatureType_ArrayDataType_FLOAT32:
                return "Float32";
            case Specification::ArrayFeatureType_ArrayDataType_INVALID_ARRAY_DATA_TYPE:
            case Specification::ArrayFeatureType_ArrayDataType_ArrayFeatureType_ArrayDataType_INT_MAX_SENTINEL_DO_NOT_USE_:
            case Specification::ArrayFeatureType_ArrayDataType_ArrayFeatureType_ArrayDataType_INT_MIN_SENTINEL_DO_NOT_USE_:
                return "Invalid";
        }
    }

    static std::string sequenceTypeToString(Specification::SequenceFeatureType::TypeCase seqType) {
        switch (seqType) {
            case Specification::SequenceFeatureType::kInt64Type:
                return "Int64";
            case Specification::SequenceFeatureType::kStringType:
                return "String";
            case Specification::SequenceFeatureType::TYPE_NOT_SET:
                return "Invalid";
        }
    }

    static std::string colorSpaceToString(Specification::ImageFeatureType_ColorSpace colorspace) {
        switch (colorspace) {
            case Specification::ImageFeatureType_ColorSpace_BGR:
                return "BGR";
            case Specification::ImageFeatureType_ColorSpace_RGB:
                return "RGB";
            case Specification::ImageFeatureType_ColorSpace_GRAYSCALE:
                return "Grayscale";
            case Specification::ImageFeatureType_ColorSpace_ImageFeatureType_ColorSpace_INT_MAX_SENTINEL_DO_NOT_USE_:
            case Specification::ImageFeatureType_ColorSpace_ImageFeatureType_ColorSpace_INT_MIN_SENTINEL_DO_NOT_USE_:
            case Specification::ImageFeatureType_ColorSpace_INVALID_COLOR_SPACE:
                return "Invalid";
        }
    }

    static std::vector<int64_t> defaultShapeOf(const Specification::ArrayFeatureType &params) {
        std::vector<int64_t> defaultShape;
        if (params.shape_size() > 0) {
            for (int i = 0; i<params.shape_size(); i++) {
                defaultShape.push_back((int64_t)params.shape(i));
            }
            return defaultShape;
        }

        switch (params.ShapeFlexibility_case()) {
            case Specification::ArrayFeatureType::kEnumeratedShapes: {
                for (int i = 0; i < params.enumeratedshapes().shapes(0).shape_size(); i++) {
                    defaultShape.push_back((int64_t)params.enumeratedshapes().shapes(0).shape(i));
                }
                break;
            }
            case Specification::ArrayFeatureType::kShapeRange: {
                for (int i=0; i < params.shaperange().sizeranges_size(); i++) {
                    defaultShape.push_back((int64_t)params.shaperange().sizeranges(i).lowerbound());
                }
                break;
            }
            case Specification::ArrayFeatureType::SHAPEFLEXIBILITY_NOT_SET:
                break;
        }

        return defaultShape;
    }

    static std::vector<int64_t> defaultSizeOf(const Specification::ImageFeatureType &params) {
        std::vector<int64_t> defaultSize;

        if (params.width() > 0 && params.height() > 0) {
            defaultSize.push_back((int64_t)params.width());
            defaultSize.push_back((int64_t)params.height());
            return defaultSize;
        }

        switch (params.SizeFlexibility_case()) {
            case Specification::ImageFeatureType::kEnumeratedSizes:
                defaultSize.push_back((int64_t)params.enumeratedsizes().sizes(0).width());
                defaultSize.push_back((int64_t)params.enumeratedsizes().sizes(0).height());
                break;
            case Specification::ImageFeatureType::kImageSizeRange:
                defaultSize.push_back((int64_t)params.imagesizerange().widthrange().lowerbound());
                defaultSize.push_back((int64_t)params.imagesizerange().heightrange().lowerbound());
                break;
            case Specification::ArrayFeatureType::SHAPEFLEXIBILITY_NOT_SET:
                break;
        }
        return defaultSize;
    }

    static std::vector<std::vector<int64_t>> enumeratedSizesOf(const Specification::ImageFeatureType &params) {
        std::vector<std::vector<int64_t>> sizes;
        for (int i=0; i<params.enumeratedsizes().sizes_size(); i++) {
            std::vector<int64_t> size;
            size.push_back((int64_t)params.enumeratedsizes().sizes(i).width());
            size.push_back((int64_t)params.enumeratedsizes().sizes(i).height());
            sizes.push_back(size);
        }
        return sizes;
    }

    static std::vector<std::pair<int64_t,int64_t>> sizeRangesOf(const Specification::ImageFeatureType &params) {
        std::vector<std::pair<int64_t,int64_t>> ranges;

        ranges.push_back(std::make_pair((int64_t)params.imagesizerange().widthrange().lowerbound(),
                                        (int64_t)params.imagesizerange().widthrange().upperbound()));
        ranges.push_back(std::make_pair((int64_t)params.imagesizerange().heightrange().lowerbound(),
                                        (int64_t)params.imagesizerange().heightrange().upperbound()));
        return ranges;
    }

    static std::vector<std::vector<int64_t>> enumeratedShapesOf(const Specification::ArrayFeatureType &params) {
        std::vector<std::vector<int64_t>> shapes;
        for (int i=0; i<params.enumeratedshapes().shapes_size(); i++) {
            std::vector<int64_t> shape;
            for (int d=0; d<params.enumeratedshapes().shapes(i).shape_size(); d++) {
                shape.push_back((int64_t)params.enumeratedshapes().shapes(i).shape(d));
            }
            shapes.push_back(shape);
        }
        return shapes;
    }

    static std::vector<std::pair<int64_t,int64_t>> shapeRangesOf(const Specification::ArrayFeatureType &params) {
        std::vector<std::pair<int64_t,int64_t>> ranges;
        for (int i=0; i<params.shaperange().sizeranges_size(); i++) {
            ranges.push_back(std::make_pair((int64_t)params.shaperange().sizeranges(i).lowerbound(),
                                            (int64_t)params.shaperange().sizeranges(i).upperbound()));
        }
        return ranges;
    }

    static std::string dimensionsToString(const std::vector<int64_t> &dims, bool useArrayFormat = false) {

        std::stringstream ss;
        std::string separator = useArrayFormat ? ", " : " x ";

        if (useArrayFormat) {
            ss << "[";
        }

        for (size_t i=0; i<dims.size(); i++) {
            ss << dims[i];
            if (i < dims.size() - 1) { ss << separator; }
        }

        if (useArrayFormat) {
            ss << "]";
        }

        return ss.str();
    }

    static std::string enumeratedShapesToString(const std::vector<std::vector<int64_t>> &enumerated, bool useArrayFormat = false) {
        std::stringstream ss;
        std::string separator = useArrayFormat ? ", " : " | ";

        if (useArrayFormat) {
            ss << "[";
        }

        for (size_t i=0; i<enumerated.size(); i++) {
            ss << dimensionsToString(enumerated[i],useArrayFormat);
            if (i < enumerated.size() - 1) { ss << separator; }
        }

        if (useArrayFormat) {
            ss << "]";
        }

        return ss.str();
    }

    static std::string rangeToString(int64_t min, int64_t max, bool useArrayFormat = false) {
        std::stringstream ss;

        if (useArrayFormat) {
            ss << "[" << min << ", " << max << "]";
        } else {
            if (min == max) {
                ss << min;
            } else if (max < 0) {
                ss << min << "...";
            } else {
                ss << min << "..." << max;
            }
        }
        return ss.str();
    }

    static std::string dimensionRangesToString(const std::vector<std::pair<int64_t,int64_t>> &rangePerDimension, bool useArrayFormat = false) {
        std::stringstream ss;
        std::string separator = useArrayFormat ? ", " : " x ";

        if (useArrayFormat) {
            ss << "[";
        }

        for (size_t i = 0; i < rangePerDimension.size(); i++) {
            ss << rangeToString(rangePerDimension[i].first,rangePerDimension[i].second,useArrayFormat);
            if (i < rangePerDimension.size() - 1) { ss << separator; }
        }

        if (useArrayFormat) {
            ss << "]";
        }

        return ss.str();
    }

    // methods
    std::string FeatureType::toString() const {
        std::stringstream ss;
        Specification::FeatureType::TypeCase tag = m_type->Type_case();

        ss << featureTypeToString(tag);

        switch (tag) {
            case Specification::FeatureType::kMultiArrayType:
            {
                const Specification::ArrayFeatureType& params = m_type->multiarraytype();
                ss << " (" << dataTypeToString(params.datatype());
                std::vector<int64_t> shape = defaultShapeOf(params);
                if (shape.size() > 0) {
                    ss << " ";
                    ss << dimensionsToString(shape);
                }
                ss << ")";
                break;
            }
            case Specification::FeatureType::kDictionaryType:
            {
                const Specification::DictionaryFeatureType& params = m_type->dictionarytype();
                ss << " (";
                ss << keyTypeToString(params.KeyType_case());
                ss << " → ";
                ss << featureTypeToString(Specification::FeatureType::kDoubleType); // assume double value
                ss << ")";
                break;
            }
            case Specification::FeatureType::kImageType:
            {
                const Specification::ImageFeatureType& params = m_type->imagetype();
                ss << " (";
                ss << colorSpaceToString(params.colorspace());
                std::vector<int64_t> size = defaultSizeOf(params);
                if (size.size() > 0) {
                    ss << " ";
                    ss << dimensionsToString(size);
                }
                ss << ")";
                break;
            }
            case Specification::FeatureType::kSequenceType:
            {
                const Specification::SequenceFeatureType& params = m_type->sequencetype();
                ss << " (";
                ss << sequenceTypeToString(params.Type_case());
                ss << " " << rangeToString((int64_t)params.sizerange().lowerbound(),params.sizerange().upperbound());
                ss << ")";
                break;
            }
            default:
                break;
        }
        return ss.str() + (m_type->isoptional() ? "?" : "");
    }

    std::map<std::string,std::string> FeatureType::toDictionary() const {
        Specification::FeatureType::TypeCase tag = m_type->Type_case();

        std::map<std::string, std::string> dict;
        dict["type"] = featureTypeToString(tag);
        dict["isOptional"] = m_type->isoptional() ? "1" : "0";

        switch (tag) {
            case Specification::FeatureType::kMultiArrayType:
            {
                const Specification::ArrayFeatureType& params = m_type->multiarraytype();
                dict["dataType"] = dataTypeToString(params.datatype());
                dict["shape"] = dimensionsToString(defaultShapeOf(params),true);
                dict["hasShapeFlexibility"] = params.ShapeFlexibility_case() != Specification::ArrayFeatureType::SHAPEFLEXIBILITY_NOT_SET ? "1" : "0";
                switch (params.ShapeFlexibility_case()) {
                    case Specification::ArrayFeatureType::kEnumeratedShapes: {
                        std::vector<std::vector<int64_t>> shapes = enumeratedShapesOf(params);
                        dict["enumeratedShapes"] = enumeratedShapesToString(shapes,true);
                        dict["shapeFlexibility"] = enumeratedShapesToString(shapes,false);
                        break;
                    }
                    case Specification::ArrayFeatureType::kShapeRange: {
                        std::vector<std::pair<int64_t,int64_t>> ranges = shapeRangesOf(params);
                        dict["shapeRange"] = dimensionRangesToString(ranges,true);
                        dict["shapeFlexibility"] = dimensionRangesToString(ranges,false);
                        break;
                    }
                    case Specification::ArrayFeatureType::SHAPEFLEXIBILITY_NOT_SET:
                        break;
                }
                break;
            }
            case Specification::FeatureType::kDictionaryType:
            {
                dict["keyType"] = keyTypeToString(m_type->dictionarytype().KeyType_case());
                break;
            }
            case Specification::FeatureType::kImageType:
            {
                const Specification::ImageFeatureType& params = m_type->imagetype();
                auto defaultSize = defaultSizeOf(params);
                dict["width"] = std::to_string(defaultSize[0]);
                dict["height"] = std::to_string(defaultSize[1]);
                dict["colorspace"] = colorSpaceToString(params.colorspace());
                dict["isColor"] = params.colorspace() == Specification::ImageFeatureType_ColorSpace_GRAYSCALE ? "0" : "1";
                dict["hasSizeFlexibility"] = params.SizeFlexibility_case() != Specification::ImageFeatureType::SIZEFLEXIBILITY_NOT_SET ? "1" : "0";
                switch (params.SizeFlexibility_case()) {
                    case Specification::ImageFeatureType::kEnumeratedSizes: {
                        std::vector<std::vector<int64_t>> shapes = enumeratedSizesOf(params);
                        dict["enumeratedSizes"] = enumeratedShapesToString(shapes,true);
                        dict["sizeFlexibility"] = enumeratedShapesToString(shapes,false);
                        break;
                    }
                    case Specification::ImageFeatureType::kImageSizeRange: {
                        std::vector<std::pair<int64_t,int64_t>> ranges = sizeRangesOf(params);
                        dict["sizeRange"] = dimensionRangesToString(ranges,true);
                        dict["sizeFlexibility"] = dimensionRangesToString(ranges,false);
                        break;
                    }
                    case Specification::ImageFeatureType::SIZEFLEXIBILITY_NOT_SET:
                        break;
                }
                break;
            }
            case Specification::FeatureType::kSequenceType:
            {
                const Specification::SequenceFeatureType& params = m_type->sequencetype();
                dict["valueType"] = sequenceTypeToString(params.Type_case());
                dict["sizeRange"] = rangeToString((int64_t)params.sizerange().lowerbound(), params.sizerange().upperbound(),true);
                break;
            }
            default:
                break;
        }

        return dict;
    }

    Specification::FeatureType* FeatureType::allocateCopy() {
        // we call new here, but don't free!
        // this method should only be called immediately prior to passing the
        // returned pointer into a protobuf method that expects to take ownership
        // over the heap object pointed to.
        return new Specification::FeatureType(*m_type);
    }

}
