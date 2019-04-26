#ifndef MLMODEL_DATA_TYPE
#define MLMODEL_DATA_TYPE

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "../build/format/FeatureTypes_enums.h"

namespace CoreML {
    namespace Specification {
        class FeatureType;
    }

    /*
     * Wrapper for Specification::DataType to enable high level logic around the proto
     * struct. Ex: conversion to string for error messages, comparison logic, etc.
     */
    class FeatureType {
    private:
        FeatureType(MLFeatureTypeType type);
        std::shared_ptr<Specification::FeatureType> m_type;

    public:
        // not explicit -- allows Specification::DataType to be passed as DataType
        FeatureType(const Specification::FeatureType& wrapped);

        // non-parametric types
        static FeatureType Int64();
        static FeatureType Double();
        static FeatureType String();
        static FeatureType Image();

        // parametric types
        static FeatureType Array(const std::vector<int64_t> dimensions);
        static FeatureType Array(const std::vector<int64_t> dimensions, MLArrayDataType dataType);
        static FeatureType Dictionary(MLDictionaryFeatureTypeKeyType keyType);

        // operators
        bool operator==(const FeatureType& other) const;
        bool operator!=(const FeatureType& other) const;
        const Specification::FeatureType& operator*() const;
        Specification::FeatureType& operator*();
        const Specification::FeatureType* operator->() const;
        Specification::FeatureType* operator->();

        // methods
        std::string toString() const;
        std::map<std::string,std::string> toDictionary() const;

        // copies onto the heap (using operator new) -- if passed into a protobuf
        // set_allocated_ method, protobuf will take ownership.
        // see https://developers.google.com/protocol-buffers/docs/reference/cpp-generated
        Specification::FeatureType* allocateCopy();
  };

}

#endif
