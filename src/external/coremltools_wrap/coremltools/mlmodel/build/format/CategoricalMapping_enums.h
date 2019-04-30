#ifndef __CATEGORICALMAPPING_ENUMS_H
#define __CATEGORICALMAPPING_ENUMS_H
enum MLCategoricalMappingMappingType: int {
    MLCategoricalMappingMappingType_stringToInt64Map = 1,
    MLCategoricalMappingMappingType_int64ToStringMap = 2,
    MLCategoricalMappingMappingType_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLCategoricalMappingMappingType_Name(MLCategoricalMappingMappingType x) {
    switch (x) {
        case MLCategoricalMappingMappingType_stringToInt64Map:
            return "MLCategoricalMappingMappingType_stringToInt64Map";
        case MLCategoricalMappingMappingType_int64ToStringMap:
            return "MLCategoricalMappingMappingType_int64ToStringMap";
        case MLCategoricalMappingMappingType_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

enum MLCategoricalMappingValueOnUnknown: int {
    MLCategoricalMappingValueOnUnknown_strValue = 101,
    MLCategoricalMappingValueOnUnknown_int64Value = 102,
    MLCategoricalMappingValueOnUnknown_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLCategoricalMappingValueOnUnknown_Name(MLCategoricalMappingValueOnUnknown x) {
    switch (x) {
        case MLCategoricalMappingValueOnUnknown_strValue:
            return "MLCategoricalMappingValueOnUnknown_strValue";
        case MLCategoricalMappingValueOnUnknown_int64Value:
            return "MLCategoricalMappingValueOnUnknown_int64Value";
        case MLCategoricalMappingValueOnUnknown_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

#endif
