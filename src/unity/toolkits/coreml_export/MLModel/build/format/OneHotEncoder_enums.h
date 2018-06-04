#ifndef __ONEHOTENCODER_ENUMS_H
#define __ONEHOTENCODER_ENUMS_H
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
enum MLHandleUnknown: int {
    MLHandleUnknownErrorOnUnknown = 0,
    MLHandleUnknownIgnoreUnknown = 1,
};

enum MLOneHotEncoderCategoryType: int {
    MLOneHotEncoderCategoryType_stringCategories = 1,
    MLOneHotEncoderCategoryType_int64Categories = 2,
    MLOneHotEncoderCategoryType_NOT_SET = 0,
};

static const char * MLOneHotEncoderCategoryType_Name(MLOneHotEncoderCategoryType x) {
    switch (x) {
        case MLOneHotEncoderCategoryType_stringCategories:
            return "MLOneHotEncoderCategoryType_stringCategories";
        case MLOneHotEncoderCategoryType_int64Categories:
            return "MLOneHotEncoderCategoryType_int64Categories";
        case MLOneHotEncoderCategoryType_NOT_SET:
            return "INVALID";
    }
}

#pragma clang diagnostic pop
#endif
