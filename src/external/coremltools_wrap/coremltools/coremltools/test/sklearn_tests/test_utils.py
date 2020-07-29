# Copyright (c) 2017, Apple Inc. All rights reserved.
# # Use of this source code is governed by a BSD-3-clause license that can be # found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import unittest
from coremltools.models.utils import rename_feature, _macos_version, _is_macos
from coremltools.models import MLModel
from coremltools._deps import _HAS_SKLEARN
import pandas as pd

if _HAS_SKLEARN:
    from sklearn.preprocessing import OneHotEncoder
    from sklearn.datasets import load_boston
    from sklearn.linear_model import LinearRegression
    from sklearn.pipeline import Pipeline
    from coremltools.converters import sklearn as converter


@unittest.skipIf(not _HAS_SKLEARN, "Missing scikit-learn. Skipping tests.")
class PipeLineRenameTests(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        scikit_data = load_boston()
        feature_names = scikit_data.feature_names

        scikit_model = LinearRegression()
        scikit_model.fit(scikit_data["data"], scikit_data["target"])

        # Save the data and the model
        self.scikit_data = scikit_data
        self.scikit_model = scikit_model

    def test_pipeline_rename(self):
        # Convert
        scikit_spec = converter.convert(self.scikit_model).get_spec()
        model = MLModel(scikit_spec)
        sample_data = self.scikit_data.data[0]

        # Rename
        rename_feature(scikit_spec, "input", "renamed_input")
        renamed_model = MLModel(scikit_spec)

        # Check the predictions
        if _is_macos() and _macos_version() >= (10, 13):
            out_dict = model.predict({"input": sample_data})
            out_dict_renamed = renamed_model.predict({"renamed_input": sample_data})
            self.assertAlmostEqual(list(out_dict.keys()), list(out_dict_renamed.keys()))
            self.assertAlmostEqual(
                list(out_dict.values()), list(out_dict_renamed.values())
            )
