# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import itertools
import pandas as pd
import os
import unittest

from coremltools._deps import _HAS_SKLEARN
from coremltools.converters.sklearn import convert
from coremltools.models.utils import (
    evaluate_classifier,
    evaluate_classifier_with_probabilities,
    _macos_version,
    _is_macos,
)

if _HAS_SKLEARN:
    from sklearn.linear_model import LogisticRegression
    from sklearn.svm import LinearSVC


@unittest.skipIf(not _HAS_SKLEARN, "Missing sklearn. Skipping tests.")
class GlmCassifierTest(unittest.TestCase):
    def test_logistic_regression_binary_classification_with_string_labels(self):
        self._conversion_and_evaluation_helper_for_logistic_regression(["Foo", "Bar"])

    def test_logistic_regression_multiclass_classification_with_int_labels(self):
        self._conversion_and_evaluation_helper_for_logistic_regression([1, 2, 3, 4])

    @staticmethod
    def _generate_random_data(labels):
        import random

        random.seed(42)

        # Generate some random data
        x, y = [], []
        for _ in range(100):
            x.append([random.gauss(2, 3), random.gauss(-1, 2)])
            y.append(random.choice(labels))
        return x, y

    def _conversion_and_evaluation_helper_for_logistic_regression(self, class_labels):
        options = {
            "C": (0.1, 1.0, 2.0),
            "fit_intercept": (True, False),
            "class_weight": ("balanced", None),
            "solver": ("newton-cg", "lbfgs", "liblinear", "sag"),
        }

        # Generate a list of all combinations of options and the default parameters
        product = itertools.product(*options.values())
        args = [{}] + [dict(zip(options.keys(), p)) for p in product]

        x, y = GlmCassifierTest._generate_random_data(class_labels)
        column_names = ["x1", "x2"]
        df = pd.DataFrame(x, columns=column_names)

        for cur_args in args:
            print(class_labels, cur_args)
            cur_model = LogisticRegression(**cur_args)
            cur_model.fit(x, y)

            spec = convert(
                cur_model, input_features=column_names, output_feature_names="target"
            )

            if _is_macos() and _macos_version() >= (10, 13):
                probability_lists = cur_model.predict_proba(x)
                df["classProbability"] = [
                    dict(zip(cur_model.classes_, cur_vals))
                    for cur_vals in probability_lists
                ]

                metrics = evaluate_classifier_with_probabilities(
                    spec, df, probabilities="classProbability", verbose=False
                )
                self.assertEquals(metrics["num_key_mismatch"], 0)
                self.assertLess(metrics["max_probability_error"], 0.00001)

    def test_linear_svc_binary_classification_with_string_labels(self):
        self._conversion_and_evaluation_helper_for_linear_svc(["Foo", "Bar"])

    def test_linear_svc_multiclass_classification_with_int_labels(self):
        self._conversion_and_evaluation_helper_for_linear_svc([1, 2, 3, 4])

    def _conversion_and_evaluation_helper_for_linear_svc(self, class_labels):
        ARGS = [
            {},
            {"C": 0.75, "loss": "hinge"},
            {"penalty": "l1", "dual": False},
            {"tol": 0.001, "fit_intercept": False},
            {"intercept_scaling": 1.5},
        ]

        x, y = GlmCassifierTest._generate_random_data(class_labels)
        column_names = ["x1", "x2"]
        df = pd.DataFrame(x, columns=column_names)

        for cur_args in ARGS:
            print(class_labels, cur_args)
            cur_model = LinearSVC(**cur_args)
            cur_model.fit(x, y)

            spec = convert(
                cur_model, input_features=column_names, output_feature_names="target"
            )

            if _is_macos() and _macos_version() >= (10, 13):
                df["prediction"] = cur_model.predict(x)

                cur_eval_metics = evaluate_classifier(spec, df, verbose=False)
                self.assertEquals(cur_eval_metics["num_errors"], 0)
