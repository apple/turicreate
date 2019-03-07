#ifndef __GLMREGRESSOR_ENUMS_H
#define __GLMREGRESSOR_ENUMS_H
enum MLPostEvaluationTransform: int {
    MLPostEvaluationTransformNoTransform = 0,
    MLPostEvaluationTransformLogit = 1,
    MLPostEvaluationTransformProbit = 2,
};

#endif
