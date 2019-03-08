#ifndef __IMPUTER_ENUMS_H
#define __IMPUTER_ENUMS_H
enum MLImputerImputedValue: int {
    MLImputerImputedValue_imputedDoubleValue = 1,
    MLImputerImputedValue_imputedInt64Value = 2,
    MLImputerImputedValue_imputedStringValue = 3,
    MLImputerImputedValue_imputedDoubleArray = 4,
    MLImputerImputedValue_imputedInt64Array = 5,
    MLImputerImputedValue_imputedStringDictionary = 6,
    MLImputerImputedValue_imputedInt64Dictionary = 7,
    MLImputerImputedValue_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLImputerImputedValue_Name(MLImputerImputedValue x) {
    switch (x) {
        case MLImputerImputedValue_imputedDoubleValue:
            return "MLImputerImputedValue_imputedDoubleValue";
        case MLImputerImputedValue_imputedInt64Value:
            return "MLImputerImputedValue_imputedInt64Value";
        case MLImputerImputedValue_imputedStringValue:
            return "MLImputerImputedValue_imputedStringValue";
        case MLImputerImputedValue_imputedDoubleArray:
            return "MLImputerImputedValue_imputedDoubleArray";
        case MLImputerImputedValue_imputedInt64Array:
            return "MLImputerImputedValue_imputedInt64Array";
        case MLImputerImputedValue_imputedStringDictionary:
            return "MLImputerImputedValue_imputedStringDictionary";
        case MLImputerImputedValue_imputedInt64Dictionary:
            return "MLImputerImputedValue_imputedInt64Dictionary";
        case MLImputerImputedValue_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

enum MLImputerReplaceValue: int {
    MLImputerReplaceValue_replaceDoubleValue = 11,
    MLImputerReplaceValue_replaceInt64Value = 12,
    MLImputerReplaceValue_replaceStringValue = 13,
    MLImputerReplaceValue_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLImputerReplaceValue_Name(MLImputerReplaceValue x) {
    switch (x) {
        case MLImputerReplaceValue_replaceDoubleValue:
            return "MLImputerReplaceValue_replaceDoubleValue";
        case MLImputerReplaceValue_replaceInt64Value:
            return "MLImputerReplaceValue_replaceInt64Value";
        case MLImputerReplaceValue_replaceStringValue:
            return "MLImputerReplaceValue_replaceStringValue";
        case MLImputerReplaceValue_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

#endif
