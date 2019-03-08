# Copyright (c) 2017, Apple Inc. All rights reserved.
# # Use of this source code is governed by a BSD-3-clause license that can be # found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import unittest
from coremltools.models.utils import rename_feature, macos_version
from coremltools.models import MLModel
from coremltools._deps import HAS_SKLEARN
import pandas as pd

if HAS_SKLEARN:
    from sklearn.preprocessing import OneHotEncoder
    from sklearn.datasets import load_boston
    from sklearn.linear_model import LinearRegression
    from sklearn.pipeline import Pipeline
    from coremltools.converters import sklearn as converter


@unittest.skipIf(not HAS_SKLEARN, 'Missing scikit-learn. Skipping tests.')
class PipeLineRenameTests(unittest.TestCase):
    """
    Unit test class for testing scikit-learn converter.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading the dataset and training a model.
        """

        if not(HAS_SKLEARN):
            return

        scikit_data = load_boston()
        feature_names = scikit_data.feature_names

        scikit_model = LinearRegression()
        scikit_model.fit(scikit_data['data'], scikit_data['target'])

        # Save the data and the model
        self.scikit_data = scikit_data
        self.scikit_model = scikit_model

    def test_pipeline_rename(self):

        # Convert
        scikit_spec = converter.convert(self.scikit_model).get_spec()
        model = MLModel(scikit_spec)
        sample_data = self.scikit_data.data[0]

        # Rename
        rename_feature(scikit_spec, 'input', 'renamed_input')
        renamed_model = MLModel(scikit_spec)

        # Check the predictions
        if macos_version() >= (10, 13):
            self.assertEquals(model.predict({'input': sample_data}),
                              renamed_model.predict({'renamed_input': sample_data}))
