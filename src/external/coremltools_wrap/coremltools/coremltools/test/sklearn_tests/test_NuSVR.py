# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import pandas as pd
import tempfile
import random
import unittest
import pytest

from coremltools._deps import (
    _HAS_LIBSVM,
    MSG_LIBSVM_NOT_FOUND,
    _HAS_SKLEARN,
    MSG_SKLEARN_NOT_FOUND,
)
from coremltools.models.utils import evaluate_regressor, _macos_version, _is_macos

if _HAS_LIBSVM:
    from libsvm import svmutil
    from libsvm import svm
    from coremltools.converters import libsvm

if _HAS_SKLEARN:
    from sklearn.svm import NuSVR
    from sklearn.datasets import load_boston
    from sklearn.preprocessing import OneHotEncoder
    from coremltools.converters import sklearn as scikit_converter


@unittest.skipIf(not _HAS_SKLEARN, MSG_SKLEARN_NOT_FOUND)
class NuSVRScikitTest(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading the dataset and training a model.
        """
        if not _HAS_SKLEARN:
            return

        self.scikit_model = NuSVR(kernel="linear")
        self.data = load_boston()
        self.scikit_model.fit(self.data["data"], self.data["target"])

    def test_conversion_bad_inputs(self):
        # Error on converting an untrained model
        with self.assertRaises(TypeError):
            model = NuSVR()
            spec = scikit_converter.convert(model, "data", "out")

        # Check the expected class during covnersion.
        with self.assertRaises(TypeError):
            model = OneHotEncoder()
            spec = scikit_converter.convert(model, "data", "out")

    @pytest.mark.slow
    def test_evaluation_stress_test(self):
        self._test_evaluation(allow_slow=True)

    def test_evaluation(self):
        self._test_evaluation(allow_slow=False)

    def _test_evaluation(self, allow_slow):
        """
        Test that the same predictions are made
        """

        # Generate some smallish (some kernels take too long on anything else) random data
        x, y = [], []
        for _ in range(50):
            cur_x1, cur_x2 = random.gauss(2, 3), random.gauss(-1, 2)
            x.append([cur_x1, cur_x2])
            y.append(1 + 2 * cur_x1 + 3 * cur_x2)

        input_names = ["x1", "x2"]
        df = pd.DataFrame(x, columns=input_names)

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
            {"C": 0.5, "shrinking": False, "nu": 0.9},
        ]

        # Test
        for param1 in non_kernel_parameters:
            for param2 in kernel_parameters:
                cur_params = param1.copy()
                cur_params.update(param2)

                cur_model = NuSVR(**cur_params)
                cur_model.fit(x, y)
                df["prediction"] = cur_model.predict(x)

                spec = scikit_converter.convert(cur_model, input_names, "target")

                if _is_macos() and _macos_version() >= (10, 13):
                    metrics = evaluate_regressor(spec, df)
                    self.assertAlmostEquals(metrics["max_error"], 0)

                if not allow_slow:
                    break

            if not allow_slow:
                break


@unittest.skipIf(not _HAS_LIBSVM, MSG_LIBSVM_NOT_FOUND)
@unittest.skipIf(not _HAS_SKLEARN, MSG_SKLEARN_NOT_FOUND)
class NuSVRLibSVMTest(unittest.TestCase):
    """
    Unit test class for testing the libsvm sklearn converter.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading the dataset and training a model.
        """
        if not _HAS_SKLEARN:
            return
        if not _HAS_LIBSVM:
            return

        scikit_data = load_boston()
        prob = svmutil.svm_problem(scikit_data["target"], scikit_data["data"].tolist())
        param = svmutil.svm_parameter()
        param.svm_type = svmutil.NU_SVR
        param.kernel_type = svmutil.LINEAR
        param.eps = 1

        self.libsvm_model = svmutil.svm_train(prob, param)

    def test_conversion(self):
        spec = libsvm.convert(self.libsvm_model, "data", "target")

    def test_conversion_from_filesystem(self):
        libsvm_model_path = tempfile.mktemp(suffix="model.libsvm")
        svmutil.svm_save_model(libsvm_model_path, self.libsvm_model)
        spec = libsvm.convert(libsvm_model_path, "data", "target")

    def test_conversion_bad_inputs(self):
        # Check the expected class during covnersion.
        with self.assertRaises(TypeError):
            model = OneHotEncoder()
            spec = libsvm.convert(model, "data", "out")

    @pytest.mark.slow
    def test_evaluation_stress_test(self):
        self._test_evaluation(allow_slow=True)

    def test_evaluation(self):
        self._test_evaluation(allow_slow=False)

    def _test_evaluation(self, allow_slow):
        """
        Test that the same predictions are made
        """
        from svm import svm_parameter, svm_problem
        from svmutil import svm_train, svm_predict

        # Generate some smallish (poly kernels take too long on anything else) random data
        x, y = [], []
        for _ in range(50):
            cur_x1, cur_x2 = random.gauss(2, 3), random.gauss(-1, 2)
            x.append([cur_x1, cur_x2])
            y.append(1 + 2 * cur_x1 + 3 * cur_x2)

        input_names = ["x1", "x2"]
        df = pd.DataFrame(x, columns=input_names)
        prob = svm_problem(y, x)

        # Parameters
        base_param = "-s 4"  # model type is nu-SVR
        non_kernel_parameters = ["", "-c 1.5 -p 0.5 -h 1", "-c 0.5 -p 0.5 -h 0"]
        kernel_parameters = [
            "",
            "-t 2 -g 1.2",  # rbf kernel
            "-t 0",  # linear kernel
            "-t 1",
            "-t 1 -d 2",
            "-t 1 -g 0.75",
            "-t 1 -d 0 -g 0.9 -r 2",  # poly kernel
            "-t 3",
            "-t 3 -g 1.3",
            "-t 3 -r 0.8",
            "-t 3 -r 0.8 -g 0.5",  # sigmoid kernel
        ]

        for param1 in non_kernel_parameters:
            for param2 in kernel_parameters:
                param_str = " ".join([base_param, param1, param2])
                param = svm_parameter(param_str)

                model = svm_train(prob, param)
                (df["prediction"], _, _) = svm_predict(y, x, model)

                spec = libsvm.convert(model, input_names, "target")

                if _is_macos() and _macos_version() >= (10, 13):
                    metrics = evaluate_regressor(spec, df)
                    self.assertAlmostEquals(metrics["max_error"], 0)

                if not allow_slow:
                    break

            if not allow_slow:
                break
