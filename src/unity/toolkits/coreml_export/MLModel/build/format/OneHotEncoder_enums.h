/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
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
