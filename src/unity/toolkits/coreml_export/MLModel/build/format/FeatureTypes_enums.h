#ifndef __FEATURETYPES_ENUMS_H
#define __FEATURETYPES_ENUMS_H
enum MLColorSpace: int {
    MLColorSpaceINVALID_COLOR_SPACE = 0,
    MLColorSpaceGRAYSCALE = 10,
    MLColorSpaceRGB = 20,
    MLColorSpaceBGR = 30,
};

enum MLImageFeatureTypeSizeFlexibility: int {
    MLImageFeatureTypeSizeFlexibility_enumeratedSizes = 21,
    MLImageFeatureTypeSizeFlexibility_imageSizeRange = 31,
    MLImageFeatureTypeSizeFlexibility_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLImageFeatureTypeSizeFlexibility_Name(MLImageFeatureTypeSizeFlexibility x) {
    switch (x) {
        case MLImageFeatureTypeSizeFlexibility_enumeratedSizes:
            return "MLImageFeatureTypeSizeFlexibility_enumeratedSizes";
        case MLImageFeatureTypeSizeFlexibility_imageSizeRange:
            return "MLImageFeatureTypeSizeFlexibility_imageSizeRange";
        case MLImageFeatureTypeSizeFlexibility_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

enum MLArrayDataType: int {
    MLArrayDataTypeINVALID_ARRAY_DATA_TYPE = 0,
    MLArrayDataTypeFLOAT32 = 65568,
    MLArrayDataTypeDOUBLE = 65600,
    MLArrayDataTypeINT32 = 131104,
};

enum MLArrayFeatureTypeShapeFlexibility: int {
    MLArrayFeatureTypeShapeFlexibility_enumeratedShapes = 21,
    MLArrayFeatureTypeShapeFlexibility_shapeRange = 31,
    MLArrayFeatureTypeShapeFlexibility_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLArrayFeatureTypeShapeFlexibility_Name(MLArrayFeatureTypeShapeFlexibility x) {
    switch (x) {
        case MLArrayFeatureTypeShapeFlexibility_enumeratedShapes:
            return "MLArrayFeatureTypeShapeFlexibility_enumeratedShapes";
        case MLArrayFeatureTypeShapeFlexibility_shapeRange:
            return "MLArrayFeatureTypeShapeFlexibility_shapeRange";
        case MLArrayFeatureTypeShapeFlexibility_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

enum MLDictionaryFeatureTypeKeyType: int {
    MLDictionaryFeatureTypeKeyType_int64KeyType = 1,
    MLDictionaryFeatureTypeKeyType_stringKeyType = 2,
    MLDictionaryFeatureTypeKeyType_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLDictionaryFeatureTypeKeyType_Name(MLDictionaryFeatureTypeKeyType x) {
    switch (x) {
        case MLDictionaryFeatureTypeKeyType_int64KeyType:
            return "MLDictionaryFeatureTypeKeyType_int64KeyType";
        case MLDictionaryFeatureTypeKeyType_stringKeyType:
            return "MLDictionaryFeatureTypeKeyType_stringKeyType";
        case MLDictionaryFeatureTypeKeyType_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

enum MLSequenceFeatureTypeType: int {
    MLSequenceFeatureTypeType_int64Type = 1,
    MLSequenceFeatureTypeType_stringType = 3,
    MLSequenceFeatureTypeType_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLSequenceFeatureTypeType_Name(MLSequenceFeatureTypeType x) {
    switch (x) {
        case MLSequenceFeatureTypeType_int64Type:
            return "MLSequenceFeatureTypeType_int64Type";
        case MLSequenceFeatureTypeType_stringType:
            return "MLSequenceFeatureTypeType_stringType";
        case MLSequenceFeatureTypeType_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

enum MLFeatureTypeType: int {
    MLFeatureTypeType_int64Type = 1,
    MLFeatureTypeType_doubleType = 2,
    MLFeatureTypeType_stringType = 3,
    MLFeatureTypeType_imageType = 4,
    MLFeatureTypeType_multiArrayType = 5,
    MLFeatureTypeType_dictionaryType = 6,
    MLFeatureTypeType_sequenceType = 7,
    MLFeatureTypeType_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLFeatureTypeType_Name(MLFeatureTypeType x) {
    switch (x) {
        case MLFeatureTypeType_int64Type:
            return "MLFeatureTypeType_int64Type";
        case MLFeatureTypeType_doubleType:
            return "MLFeatureTypeType_doubleType";
        case MLFeatureTypeType_stringType:
            return "MLFeatureTypeType_stringType";
        case MLFeatureTypeType_imageType:
            return "MLFeatureTypeType_imageType";
        case MLFeatureTypeType_multiArrayType:
            return "MLFeatureTypeType_multiArrayType";
        case MLFeatureTypeType_dictionaryType:
            return "MLFeatureTypeType_dictionaryType";
        case MLFeatureTypeType_sequenceType:
            return "MLFeatureTypeType_sequenceType";
        case MLFeatureTypeType_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

#endif
