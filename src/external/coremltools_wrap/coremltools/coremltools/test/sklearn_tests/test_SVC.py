# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import unittest
import tempfile
import os
import pandas as pd
import random
import pytest
import numpy as np

from coremltools.models.utils import (
    evaluate_classifier,
    evaluate_classifier_with_probabilities,
    _macos_version,
    _is_macos,
)
from coremltools._deps import _HAS_LIBSVM, _HAS_SKLEARN

if _HAS_SKLEARN:
    from sklearn.svm import SVC
    from coremltools.converters import sklearn as scikit_converter

if _HAS_LIBSVM:
    from svm import svm_parameter, svm_problem
    from svmutil import svm_train, svm_predict
    from coremltools.converters import libsvm
    import svmutil


@unittest.skipIf(not _HAS_SKLEARN, "Missing scikit-learn. Skipping tests.")
class SvcScikitTest(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter.
    """

    def _evaluation_test_helper(
        self,
        class_labels,
        use_probability_estimates,
        allow_slow,
        allowed_prob_delta=0.00001,
    ):
        # Parameters to test
        kernel_parameters = [
            {},
            {"kernel": "rbf", "gamma": 1.2},
            {"kernel": "linear"},
            {"kernel": "poly"},
            {"kernel": "poly", "degree": 2},
            {"kernel": "poly", "gamma": 0.75},
            {"kernel": "poly", "degree": 0, "gamma": 0.9, "coef0": 2},
            {"kernel": "sigmoid"},
            {"kernel": "sigmoid", "gamma": 1.3},
            {"kernel": "sigmoid", "coef0": 0.8},
            {"kernel": "sigmoid", "coef0": 0.8, "gamma": 0.5},
        ]
        non_kernel_parameters = [
            {},
            {"C": 1},
            {"C": 1.5, "shrinking": True},
            {"C": 0.5, "shrinking": False},
        ]

        # Generate some random data
        x, y = [], []
        random.seed(42)
        for _ in range(50):
            x.append(
                [random.gauss(200, 30), random.gauss(-100, 22), random.gauss(100, 42)]
            )
            y.append(random.choice(class_labels))
        column_names = ["x1", "x2", "x3"]
        # make sure first label is seen first, second is seen second, and so on.
        for i, val in enumerate(class_labels):
            y[i] = val
        df = pd.DataFrame(x, columns=column_names)

        # Test
        for param1 in non_kernel_parameters:
            for param2 in kernel_parameters:
                cur_params = param1.copy()
                cur_params.update(param2)
                cur_params["probability"] = use_probability_estimates
                cur_params["max_iter"] = 10  # Don't want test to take too long
                print("cur_params=" + str(cur_params))

                cur_model = SVC(**cur_params)
                cur_model.fit(x, y)

                spec = scikit_converter.convert(cur_model, column_names, "target")

                if _is_macos() and _macos_version() >= (10, 13):
                    if use_probability_estimates:
                        probability_lists = cur_model.predict_proba(x)
                        df["classProbability"] = [
                            dict(zip(cur_model.classes_, cur_vals))
                            for cur_vals in probability_lists
                        ]
                        metrics = evaluate_classifier_with_probabilities(
                            spec, df, probabilities="classProbability", verbose=True
                        )
                        self.assertEquals(metrics["num_key_mismatch"], 0)
                        self.assertLess(
                            metrics["max_probability_error"], allowed_prob_delta
                        )
                    else:
                        df["prediction"] = cur_model.predict(x)
                        metrics = evaluate_classifier(spec, df, verbose=False)
                        self.assertEquals(metrics["num_errors"], 0)

                if not allow_slow:
                    break

            if not allow_slow:
                break

    @pytest.mark.slow
    def test_binary_class_string_label_without_probability_stress_test(self):
        self._evaluation_test_helper(["A", "B"], False, allow_slow=True)

    def test_binary_class_string_label_without_probability(self):
        self._evaluation_test_helper(["A", "B"], False, allow_slow=False)

    @pytest.mark.slow
    def test_binary_class_string_label_with_probability_stress_test(self):
        # Scikit Learn uses technique to normalize pairwise probabilities even for binary classification.
        # This leads to difference in probabilities.
        self._evaluation_test_helper(
            ["foo", "bar"], True, allow_slow=True, allowed_prob_delta=0.005
        )

    def test_binary_class_string_label_with_probability(self):
        # Scikit Learn uses technique to normalize pairwise probabilities even for binary classification.
        # This leads to difference in probabilities.
        self._evaluation_test_helper(
            ["foo", "bar"], True, allow_slow=False, allowed_prob_delta=0.005
        )

    @pytest.mark.slow
    def test_multi_class_int_label_without_probability_stress_test(self):
        self._evaluation_test_helper([12, 33, -1, 1234], False, allow_slow=True)

    def test_multi_class_int_label_without_probability(self):
        self._evaluation_test_helper([12, 33, -1, 1234], False, allow_slow=False)

    @pytest.mark.slow
    def test_multi_class_int_label_with_probability_stress_test(self):
        self._evaluation_test_helper([1, 2, 3], True, allow_slow=True)

    def test_multi_class_int_label_with_probability(self):
        self._evaluation_test_helper([1, 2, 3], True, allow_slow=False)

    def test_conversion_bad_inputs(self):
        from sklearn.preprocessing import OneHotEncoder

        # Error on converting an untrained model
        with self.assertRaises(TypeError):
            model = SVC()
            spec = scikit_converter.convert(model, "data", "out")

        # Check the expected class during conversion
        with self.assertRaises(TypeError):
            model = OneHotEncoder()
            spec = scikit_converter.convert(model, "data", "out")


@unittest.skipIf(not _HAS_LIBSVM, "Missing libsvm. Skipping tests.")
class CSVCLibSVMTest(unittest.TestCase):
    # Model parameters for testing
    base_param = "-s 0 -q "  # model type C-SVC and quiet mode
    non_kernel_parameters = ["", "-c 1.5 -p 0.5 -h 1", "-c 0.5 -p 0.5 -h 0"]
    kernel_parameters = [
        "-t 0",  # linear kernel
        "",
        "-t 2 -g 1.2",  # rbf kernel
        "-t 1",
        "-t 1 -d 2",
        "-t 1 -g 0.75",
        "-t 1 -d 0 -g 0.9 -r 2",  # poly kernel
        "-t 3",
        "-t 3 -g 1.3",
        "-t 3 -r 0.8",
        "-t 3 -r 0.8 -g 0.5",  # sigmoid kernel
    ]
    # XXX: wi params?

    """
    Unit test class for testing the libsvm converter.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading the dataset and training a model.
        """
        if not _HAS_LIBSVM:
            # setUpClass is still called even if class is skipped.
            return

        # Generate some random data.
        # This unit test should not rely on scikit learn for test data.
        self.x, self.y = [], []
        random.seed(42)
        for _ in range(50):
            self.x.append([random.gauss(200, 30), random.gauss(-100, 22)])
            self.y.append(random.choice([1, 2]))
        self.y[0] = 1  # Make sure 1 is always the first label it sees
        self.y[1] = 2
        self.column_names = ["x1", "x2"]
        self.prob = svmutil.svm_problem(self.y, self.x)

        param = svmutil.svm_parameter()
        param.svm_type = svmutil.C_SVC
        param.kernel_type = svmutil.LINEAR
        param.eps = 1
        param.probability = 1

        self.libsvm_model = svmutil.svm_train(self.prob, param)

    def test_default_names(self):
        df = pd.DataFrame({"input": self.x})
        df["input"] = df["input"].apply(np.array)

        # Test with probabilities
        spec = libsvm.convert(self.libsvm_model).get_spec()
        if _is_macos() and _macos_version() >= (10, 13):
            (_, _, probability_lists) = svm_predict(
                self.y, self.x, self.libsvm_model, "-b 1 -q"
            )
            probability_dicts = [
                dict(zip([1, 2], cur_vals)) for cur_vals in probability_lists
            ]
            df["classProbability"] = probability_dicts
            metrics = evaluate_classifier_with_probabilities(
                spec, df, verbose=False, probabilities="classProbability"
            )
            self.assertLess(metrics["max_probability_error"], 0.00001)

        # Test model without probabilities
        no_probability_model = svmutil.svm_train(self.prob, svmutil.svm_parameter())
        spec = libsvm.convert(no_probability_model).get_spec()
        self.assertEqual(len(spec.description.output), 1)
        self.assertEqual(spec.description.output[0].name, u"target")
        if _is_macos() and _macos_version() >= (10, 13):
            (df["prediction"], _, _) = svm_predict(
                self.y, self.x, no_probability_model, " -q"
            )
            metrics = evaluate_classifier(spec, df, verbose=False)
            self.assertEquals(metrics["num_errors"], 0)

    # LibSVM only supports string labels
    @pytest.mark.slow
    def test_binary_class_without_probability_stress_test(self):
        self._evaluation_test_helper_no_probability([0, 1], allow_slow=True)

    @pytest.mark.slow
    def test_binary_class_with_probability_stress_test(self):
        self._evaluation_test_helper_with_probability([-1, 90], allow_slow=True)

    @pytest.mark.slow
    def test_multi_class_without_probability_stress_test(self):
        self._evaluation_test_helper_no_probability([12, 33, 12341], allow_slow=True)

    @pytest.mark.slow
    def test_multi_class_with_probability_stress_test(self):
        self._evaluation_test_helper_with_probability([1, 2, 3], allow_slow=True)

    # LibSVM only supports string labels
    def test_binary_class_without_probability(self):
        self._evaluation_test_helper_no_probability([0, 1], allow_slow=False)

    def test_binary_class_with_probability(self):
        self._evaluation_test_helper_with_probability([-1, 90], allow_slow=False)

    def test_multi_class_without_probability(self):
        self._evaluation_test_helper_no_probability([12, 33, 12341], allow_slow=False)

    def test_multi_class_with_probability(self):
        self._evaluation_test_helper_with_probability([1, 2, 3], allow_slow=False)

    def _evaluation_test_helper_with_probability(self, labels, allow_slow):
        import copy

        df = pd.DataFrame(self.x, columns=self.column_names)
        y = copy.copy(self.y)
        for i, val in enumerate(labels):
            y[i] = val
        probability_param = "-b 1"

        for param1 in self.non_kernel_parameters:
            for param2 in self.kernel_parameters:
                param_str = " ".join(
                    [self.base_param, param1, param2, probability_param]
                )
                # print("PARAMS: ", param_str)
                param = svm_parameter(param_str)

                model = svm_train(self.prob, param)

                # Get predictions with probabilities as dictionaries
                (df["prediction"], _, probability_lists) = svm_predict(
                    y, self.x, model, probability_param + " -q"
                )
                probability_dicts = [
                    dict(zip([1, 2], cur_vals)) for cur_vals in probability_lists
                ]
                df["probabilities"] = probability_dicts

                spec = libsvm.convert(
                    model, self.column_names, "target", "probabilities"
                )

                if _is_macos() and _macos_version() >= (10, 13):
                    metrics = evaluate_classifier_with_probabilities(
                        spec, df, verbose=False
                    )
                    self.assertEquals(metrics["num_key_mismatch"], 0)
                    self.assertLess(metrics["max_probability_error"], 0.00001)

                if not allow_slow:
                    break

            if not allow_slow:
                break

    def _evaluation_test_helper_no_probability(self, labels, allow_slow):
        # Generate some random data.
        # This unit test should not rely on scikit learn for test data.
        x, y = [], []
        random.seed(42)
        for _ in range(50):
            x.append(
                [random.gauss(200, 30), random.gauss(-100, 22), random.gauss(100, 42)]
            )
            y.append(random.choice(labels))
        # make sure first label is seen first, second is seen second, and so on.
        for i, val in enumerate(labels):
            y[i] = val
        column_names = ["x1", "x2", "x3"]
        prob = svmutil.svm_problem(y, x)

        df = pd.DataFrame(x, columns=column_names)

        for param1 in self.non_kernel_parameters:
            for param2 in self.kernel_parameters:
                param_str = " ".join([self.base_param, param1, param2])
                print("PARAMS: ", param_str)
                param = svm_parameter(param_str)

                model = svm_train(prob, param)

                # Get predictions with probabilities as dictionaries
                (df["prediction"], _, _) = svm_predict(y, x, model, " -q")

                spec = libsvm.convert(model, column_names, "target")

                if _is_macos() and _macos_version() >= (10, 13):
                    metrics = evaluate_classifier(spec, df, verbose=False)
                    self.assertEquals(metrics["num_errors"], 0)

                if not allow_slow:
                    break

            if not allow_slow:
                break

    def test_conversion_from_filesystem(self):
        libsvm_model_path = tempfile.mktemp(suffix="model.libsvm")
        svmutil.svm_save_model(libsvm_model_path, self.libsvm_model)
        # libsvm's save(...) truncates floating points. So it's not going to match self.libsvm_model any more.
        spec = libsvm.convert(libsvm_model_path, self.column_names, "target")
        self.assertIsNotNone(spec)
