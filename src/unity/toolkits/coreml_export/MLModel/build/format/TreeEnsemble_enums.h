#ifndef __TREEENSEMBLE_ENUMS_H
#define __TREEENSEMBLE_ENUMS_H
enum MLTreeEnsemblePostEvaluationTransform: int {
    MLTreeEnsemblePostEvaluationTransformNoTransform = 0,
    MLTreeEnsemblePostEvaluationTransformClassification_SoftMax = 1,
    MLTreeEnsemblePostEvaluationTransformRegression_Logistic = 2,
    MLTreeEnsemblePostEvaluationTransformClassification_SoftMaxWithZeroClassReference = 3,
};

enum MLTreeNodeBehavior: int {
    MLTreeNodeBehaviorBranchOnValueLessThanEqual = 0,
    MLTreeNodeBehaviorBranchOnValueLessThan = 1,
    MLTreeNodeBehaviorBranchOnValueGreaterThanEqual = 2,
    MLTreeNodeBehaviorBranchOnValueGreaterThan = 3,
    MLTreeNodeBehaviorBranchOnValueEqual = 4,
    MLTreeNodeBehaviorBranchOnValueNotEqual = 5,
    MLTreeNodeBehaviorLeafNode = 6,
};

enum MLTreeEnsembleClassifierClassLabels: int {
    MLTreeEnsembleClassifierClassLabels_stringClassLabels = 100,
    MLTreeEnsembleClassifierClassLabels_int64ClassLabels = 101,
    MLTreeEnsembleClassifierClassLabels_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLTreeEnsembleClassifierClassLabels_Name(MLTreeEnsembleClassifierClassLabels x) {
    switch (x) {
        case MLTreeEnsembleClassifierClassLabels_stringClassLabels:
            return "MLTreeEnsembleClassifierClassLabels_stringClassLabels";
        case MLTreeEnsembleClassifierClassLabels_int64ClassLabels:
            return "MLTreeEnsembleClassifierClassLabels_int64ClassLabels";
        case MLTreeEnsembleClassifierClassLabels_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

#endif
