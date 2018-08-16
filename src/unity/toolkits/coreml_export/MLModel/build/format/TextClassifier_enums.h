#ifndef __TEXTCLASSIFIER_ENUMS_H
#define __TEXTCLASSIFIER_ENUMS_H
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
enum MLTextClassifierClassLabels: int {
    MLTextClassifierClassLabels_stringClassLabels = 200,
    MLTextClassifierClassLabels_NOT_SET = 0,
};

static const char * MLTextClassifierClassLabels_Name(MLTextClassifierClassLabels x) {
    switch (x) {
        case MLTextClassifierClassLabels_stringClassLabels:
            return "MLTextClassifierClassLabels_stringClassLabels";
        case MLTextClassifierClassLabels_NOT_SET:
            return "INVALID";
    }
}

#pragma clang diagnostic pop
#endif
