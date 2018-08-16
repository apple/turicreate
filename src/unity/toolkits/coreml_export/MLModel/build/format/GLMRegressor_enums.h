#ifndef __GLMREGRESSOR_ENUMS_H
#define __GLMREGRESSOR_ENUMS_H
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
enum MLPostEvaluationTransform: int {
    MLPostEvaluationTransformNoTransform = 0,
    MLPostEvaluationTransformLogit = 1,
    MLPostEvaluationTransformProbit = 2,
};

#pragma clang diagnostic pop
#endif
