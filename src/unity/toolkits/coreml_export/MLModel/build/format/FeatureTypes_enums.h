/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef __FEATURETYPES_ENUMS_H
#define __FEATURETYPES_ENUMS_H
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
enum MLColorSpace: int {
    MLColorSpaceINVALID_COLOR_SPACE = 0,
    MLColorSpaceGRAYSCALE = 10,
    MLColorSpaceRGB = 20,
    MLColorSpaceBGR = 30,
};

enum MLArrayDataType: int {
    MLArrayDataTypeINVALID_ARRAY_DATA_TYPE = 0,
    MLArrayDataTypeFLOAT32 = 65568,
    MLArrayDataTypeDOUBLE = 65600,
    MLArrayDataTypeINT32 = 131104,
};

enum MLDictionaryFeatureTypeKeyType: int {
    MLDictionaryFeatureTypeKeyType_int64KeyType = 1,
    MLDictionaryFeatureTypeKeyType_stringKeyType = 2,
    MLDictionaryFeatureTypeKeyType_NOT_SET = 0,
};

static const char * MLDictionaryFeatureTypeKeyType_Name(MLDictionaryFeatureTypeKeyType x) {
    switch (x) {
        case MLDictionaryFeatureTypeKeyType_int64KeyType:
            return "MLDictionaryFeatureTypeKeyType_int64KeyType";
        case MLDictionaryFeatureTypeKeyType_stringKeyType:
            return "MLDictionaryFeatureTypeKeyType_stringKeyType";
        case MLDictionaryFeatureTypeKeyType_NOT_SET:
            return "INVALID";
    }
}

enum MLFeatureTypeType: int {
    MLFeatureTypeType_int64Type = 1,
    MLFeatureTypeType_doubleType = 2,
    MLFeatureTypeType_stringType = 3,
    MLFeatureTypeType_imageType = 4,
    MLFeatureTypeType_multiArrayType = 5,
    MLFeatureTypeType_dictionaryType = 6,
    MLFeatureTypeType_NOT_SET = 0,
};

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
        case MLFeatureTypeType_NOT_SET:
            return "INVALID";
    }
}

#pragma clang diagnostic pop
#endif
