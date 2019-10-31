# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from ._pre_trained_models import _get_cache_dir


def _create_feature_extractor(model_name):
    import os
    from platform import system
    from ._internal_utils import _mac_ver
    from ._pre_trained_models import IMAGE_MODELS
    from turicreate.config import get_runtime_config
    from turicreate import extensions

    # If we don't have Core ML, use a TensorFlow model.
    if system() != 'Darwin' or _mac_ver() < (10, 13):
        ptModel = IMAGE_MODELS[model_name]()
        return TensorFlowFeatureExtractor(ptModel)

    download_path = _get_cache_dir()

    result = extensions.__dict__["image_deep_feature_extractor"]()
    result.init_options({'model_name': model_name, 'download_path': download_path})
    return result


class ImageFeatureExtractor(object):

    def __init__(self, ptModel):
        """
        Parameters
        ----------
        ptModel: ImageClassifierPreTrainedModel
            An instance of a pre-trained model.
        """
        pass

    def extract_features(self, dataset, feature):
        """
        Parameters
        ----------
        dataset: SFrame
            SFrame with data to extract features from
        feature: str
            Name of the column in `dataset` containing the features
        """
        pass

    def get_coreml_model(self):
        """
        Returns
        -------
        model:
            Return the underlying model in Core ML format
        """
        pass


class TensorFlowFeatureExtractor(ImageFeatureExtractor):
    def __init__(self, ptModel):
        """
        Parameters
        ----------
        ptModel: ImageClassifierPreTrainedModel
            An instance of a pre-trained model.
        """
        from tensorflow import keras

        self.ptModel = ptModel

        self.input_shape = ptModel.input_image_shape
        self.coreml_data_layer = ptModel.coreml_data_layer
        self.coreml_feature_layer = ptModel.coreml_feature_layer

        model_path = ptModel.get_model_path('tensorflow')
        self.model = keras.models.load_model(model_path)

    def extract_features(self, dataset, feature, batch_size=64, verbose=False):
        from array import array
        import turicreate as tc

        input_is_BGR = self.ptModel.input_is_BGR

        def preprocess(image):
            pixel_data = image.pixel_data
            if input_is_BGR:
                pixel_data = pixel_data[:,:,::-1]         # Convert RGB to BGR
            return pixel_data

        images = dataset[feature]
        images = tc.image_analysis.resize(
            images,
            self.ptModel.input_image_shape[1],
            self.ptModel.input_image_shape[2],
            self.ptModel.input_image_shape[0])
        images = images.apply(preprocess)

        # TODO: use batching and an SArray builder

        images = images.to_numpy()
        y = self.model.predict(images)
        y = [i[0][0] for i in y]

        return tc.SArray(y, dtype=array)

    def get_coreml_model(self):
        import coremltools

        model_path = self.ptModel.get_model_path('coreml')
        return coremltools.models.MLModel(model_path)
