/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <iostream>
#include <exception>

#ifndef MLMODEL_DECLARE_TEST
#define MLMODEL_DECLARE_TEST(x) int x();
#endif

#ifdef MLMODEL_RUN_TEST
#define MLMODEL_TEST(x) MLMODEL_RUN_TEST(x)
#else
#define MLMODEL_TEST(x) MLMODEL_DECLARE_TEST(x)
#endif

MLMODEL_TEST(testBasicSaveLoad)
MLMODEL_TEST(testLinearModelBasic)
MLMODEL_TEST(testTreeEnsembleBasic)
MLMODEL_TEST(testOneHotEncoderBasic)
MLMODEL_TEST(testLargeModel)
MLMODEL_TEST(testVeryLargeModel)
MLMODEL_TEST(testOptionalInputs)
MLMODEL_TEST(testNNValidatorLoop)
MLMODEL_TEST(testNNValidatorMissingInput)
MLMODEL_TEST(testNNValidatorSimple)
MLMODEL_TEST(testNNValidatorMissingOutput)
MLMODEL_TEST(testNNValidatorBadInputs)
MLMODEL_TEST(testNNValidatorBadInput)
MLMODEL_TEST(testNNValidatorBadInput2)
MLMODEL_TEST(testNNValidatorBadOutput)
MLMODEL_TEST(testNNValidatorBadOutput2)
MLMODEL_TEST(testRNNLayer)
MLMODEL_TEST(testRNNLayer2)
MLMODEL_TEST(testNNValidatorAllOptional)
MLMODEL_TEST(testNNValidatorReshape3D)
MLMODEL_TEST(testNNValidatorReshape4D)
MLMODEL_TEST(testNNValidatorReshapeBad)
MLMODEL_TEST(testNNCompilerValidation)
MLMODEL_TEST(testNNCompilerValidationGoodProbBlob)
MLMODEL_TEST(testNNCompilerValidationBadProbBlob)

#undef MLMODEL_TEST
