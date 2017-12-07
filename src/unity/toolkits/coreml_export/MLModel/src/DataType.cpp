/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "Comparison.hpp"
#include "DataType.hpp"
#include "Format.hpp"

#include <protobuf/util/message_differencer.h>

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
    FeatureType FeatureType::Array(const std::vector<uint64_t> shape, MLArrayDataType dataType) {
        FeatureType out(MLFeatureTypeType_multiArrayType);
        Specification::ArrayFeatureType *params = out->mutable_multiarraytype();
        
        for (uint64_t s : shape) {
            params->add_shape(s);
        }
        params->set_datatype(static_cast<Specification::ArrayFeatureType::ArrayDataType>(dataType));
        return out;
    }

    FeatureType FeatureType::Array(const std::vector<uint64_t> shape) {
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
    
    static void dataTypeToString(std::stringstream& ss, Specification::FeatureType::TypeCase tag) {
        switch (tag) {
            case Specification::FeatureType::kMultiArrayType:
            case Specification::FeatureType::kDictionaryType:
            case Specification::FeatureType::kImageType:
                // these are parameterized; tag is insufficient to yield string representation
                assert(false);
                break;
            case Specification::FeatureType::TYPE_NOT_SET:
                assert(false);
                break;
            case Specification::FeatureType::kDoubleType:
                ss << "Double";
                break;
            case Specification::FeatureType::kInt64Type:
                ss << "Int64";
                break;
            case Specification::FeatureType::kStringType:
                ss << "String";
                break;
        }
    }
    
    
    static void keyTypeToString(std::stringstream& ss, Specification::DictionaryFeatureType::KeyTypeCase tag) {
        switch (tag) {
            case Specification::DictionaryFeatureType::KEYTYPE_NOT_SET:
                ss << "Invalid";
                break;
            case Specification::DictionaryFeatureType::kInt64KeyType:
                ss << "Int64";
                break;
            case Specification::DictionaryFeatureType::kStringKeyType:
                ss << "String";
                break;
        }
    }
    
    // methods
    std::string FeatureType::toString() const {
        std::stringstream ss;
        Specification::FeatureType::TypeCase tag = m_type->Type_case();
        
        switch (tag) {
            case Specification::FeatureType::kMultiArrayType:
            {
                const Specification::ArrayFeatureType& params = m_type->multiarraytype();
                ss << "MultiArray<";
                switch (params.datatype()) {
                    case Specification::ArrayFeatureType_ArrayDataType_INT32:
                        ss << "Int32";
                        break;
                    case Specification::ArrayFeatureType_ArrayDataType_DOUBLE:
                        ss << "Double";
                        break;
                    case Specification::ArrayFeatureType_ArrayDataType_FLOAT32:
                        ss << "Float32";
                        break;
                    case Specification::ArrayFeatureType_ArrayDataType_INVALID_ARRAY_DATA_TYPE:
                    case Specification::ArrayFeatureType_ArrayDataType_ArrayFeatureType_ArrayDataType_INT_MAX_SENTINEL_DO_NOT_USE_:
                    case Specification::ArrayFeatureType_ArrayDataType_ArrayFeatureType_ArrayDataType_INT_MIN_SENTINEL_DO_NOT_USE_:
                        ss << "Invalid";
                        break;
                }
                
                for (int i=0; i<params.shape_size(); i++) {
                    ss << ",";
                    ss << params.shape(i);
                }
                ss << ">";
                break;
            }
            case Specification::FeatureType::kDictionaryType:
            {
                const Specification::DictionaryFeatureType& params = m_type->dictionarytype();
                ss << "Dictionary<";
                keyTypeToString(ss, params.KeyType_case());
                ss << ",";
                dataTypeToString(ss, Specification::FeatureType::kDoubleType); // assume double value
                ss << ">";
                break;
            }
            case Specification::FeatureType::kImageType:
            {
                const Specification::ImageFeatureType& params = m_type->imagetype();
                ss << "Image<";
                switch (params.colorspace()) {
                    case Specification::ImageFeatureType_ColorSpace_BGR:
                        ss << "BGR";
                        break;
                    case Specification::ImageFeatureType_ColorSpace_RGB:
                        ss << "RGB";
                        break;
                    case Specification::ImageFeatureType_ColorSpace_GRAYSCALE:
                        ss << "Grayscale";
                        break;
                    case Specification::ImageFeatureType_ColorSpace_ImageFeatureType_ColorSpace_INT_MAX_SENTINEL_DO_NOT_USE_:
                    case Specification::ImageFeatureType_ColorSpace_ImageFeatureType_ColorSpace_INT_MIN_SENTINEL_DO_NOT_USE_:
                    case Specification::ImageFeatureType_ColorSpace_INVALID_COLOR_SPACE:
                        ss << "Invalid";
                        break;
                }
                ss << "," << params.width();
                ss << "," << params.height();
                ss << ">";
                break;
            }
            default:
                dataTypeToString(ss, tag);
                
        }
        return ss.str() + (m_type->isoptional() ? "?" : "");
    }
    
    Specification::FeatureType* FeatureType::allocateCopy() {
        // we call new here, but don't free!
        // this method should only be called immediately prior to passing the
        // returned pointer into a protobuf method that expects to take ownership
        // over the heap object pointed to.
        return new Specification::FeatureType(*m_type);
    }

}
