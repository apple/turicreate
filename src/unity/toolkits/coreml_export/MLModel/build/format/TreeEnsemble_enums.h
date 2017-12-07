/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef __TREEENSEMBLE_ENUMS_H
#define __TREEENSEMBLE_ENUMS_H
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
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

static const char * MLTreeEnsembleClassifierClassLabels_Name(MLTreeEnsembleClassifierClassLabels x) {
    switch (x) {
        case MLTreeEnsembleClassifierClassLabels_stringClassLabels:
            return "MLTreeEnsembleClassifierClassLabels_stringClassLabels";
        case MLTreeEnsembleClassifierClassLabels_int64ClassLabels:
            return "MLTreeEnsembleClassifierClassLabels_int64ClassLabels";
        case MLTreeEnsembleClassifierClassLabels_NOT_SET:
            return "INVALID";
    }
}

#pragma clang diagnostic pop
#endif
