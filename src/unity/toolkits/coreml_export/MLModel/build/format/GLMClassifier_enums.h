#ifndef __GLMCLASSIFIER_ENUMS_H
#define __GLMCLASSIFIER_ENUMS_H
enum MLPostEvaluationTransform: int {
    MLPostEvaluationTransformLogit = 0,
    MLPostEvaluationTransformProbit = 1,
};

enum MLClassEncoding: int {
    MLClassEncodingReferenceClass = 0,
    MLClassEncodingOneVsRest = 1,
};

enum MLGLMClassifierClassLabels: int {
    MLGLMClassifierClassLabels_stringClassLabels = 100,
    MLGLMClassifierClassLabels_int64ClassLabels = 101,
    MLGLMClassifierClassLabels_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLGLMClassifierClassLabels_Name(MLGLMClassifierClassLabels x) {
    switch (x) {
        case MLGLMClassifierClassLabels_stringClassLabels:
            return "MLGLMClassifierClassLabels_stringClassLabels";
        case MLGLMClassifierClassLabels_int64ClassLabels:
            return "MLGLMClassifierClassLabels_int64ClassLabels";
        case MLGLMClassifierClassLabels_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

#endif
