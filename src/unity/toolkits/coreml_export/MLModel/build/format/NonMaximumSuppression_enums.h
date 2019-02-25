#ifndef __NONMAXIMUMSUPPRESSION_ENUMS_H
#define __NONMAXIMUMSUPPRESSION_ENUMS_H
enum MLNonMaximumSuppressionSuppressionMethod: int {
    MLNonMaximumSuppressionSuppressionMethod_pickTop = 1,
    MLNonMaximumSuppressionSuppressionMethod_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLNonMaximumSuppressionSuppressionMethod_Name(MLNonMaximumSuppressionSuppressionMethod x) {
    switch (x) {
        case MLNonMaximumSuppressionSuppressionMethod_pickTop:
            return "MLNonMaximumSuppressionSuppressionMethod_pickTop";
        case MLNonMaximumSuppressionSuppressionMethod_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

enum MLNonMaximumSuppressionClassLabels: int {
    MLNonMaximumSuppressionClassLabels_stringClassLabels = 100,
    MLNonMaximumSuppressionClassLabels_int64ClassLabels = 101,
    MLNonMaximumSuppressionClassLabels_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLNonMaximumSuppressionClassLabels_Name(MLNonMaximumSuppressionClassLabels x) {
    switch (x) {
        case MLNonMaximumSuppressionClassLabels_stringClassLabels:
            return "MLNonMaximumSuppressionClassLabels_stringClassLabels";
        case MLNonMaximumSuppressionClassLabels_int64ClassLabels:
            return "MLNonMaximumSuppressionClassLabels_int64ClassLabels";
        case MLNonMaximumSuppressionClassLabels_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

#endif
