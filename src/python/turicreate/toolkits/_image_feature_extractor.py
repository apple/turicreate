# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from ._pre_trained_models import _get_cache_dir
import turicreate.toolkits._tf_utils as _utils

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

        # Suppresses verbosity to only errors
        _utils.suppress_tensorflow_warnings()

        from tensorflow import keras

        self.gpu_policy = _utils.TensorFlowGPUPolicy()
        self.gpu_policy.start()

        self.ptModel = ptModel

        self.input_shape = ptModel.input_image_shape
        self.coreml_data_layer = ptModel.coreml_data_layer
        self.coreml_feature_layer = ptModel.coreml_feature_layer

        model_path = ptModel.get_model_path('tensorflow')
        self.model = keras.models.load_model(model_path)

    def __del__(self):
        self.gpu_policy.stop()

    def extract_features(self, dataset, feature, batch_size=64, verbose=False):
        from array import array
        import turicreate as tc
        import numpy as np

        # Only expose the feature column to the SFrame-to-NumPy loader.
        image_sf = tc.SFrame({'image' : dataset[feature]})

        # Encapsulate state in a dict to sidestep variable scoping issues.
        state = {}
        state['num_started'] = 0       # Images read from the SFrame
        state['num_processed'] = 0     # Images processed by TensorFlow
        state['total'] = len(dataset)  # Images in the dataset

        # We should be using SArrayBuilder, but it doesn't accept ndarray yet.
        # TODO: https://github.com/apple/turicreate/issues/2672
        #out = _tc.SArrayBuilder(dtype = array.array)
        state['out'] = tc.SArray(dtype=array)

        if verbose:
            print("Performing feature extraction on resized images...")

        # Provide an iterator-like interface around the SFrame.
        def has_next_batch():
            return state['num_started'] < state['total']

        # Yield a numpy array representing one batch of images.
        def next_batch(batch):

            if not has_next_batch():
                return None

            # Compute the range of the SFrame to yield.
            start_index = state['num_started']
            end_index = min(start_index + batch_size, state['total'])
            state['num_started'] = end_index

            num_images = end_index - start_index
            shape = (num_images,) + self.ptModel.input_image_shape
            if batch.shape != shape:
                batch = np.resize(batch, shape)
            batch[:] = 0

            # Resize and load the images.
            future = tc.extensions.sframe_load_to_numpy.run_background(
                                               image_sf, batch.ctypes.data,
                                               batch.strides, batch.shape,
                                               start_index, end_index)

            return future, batch

        def ready_batch(batch_info):
            assert batch_info is not None

            batch_future, batch = batch_info
            batch_future.result()

            # TODO: Converge to NCHW everywhere.
            batch = batch.transpose(0, 2, 3, 1)  # NCHW -> NHWC

            if self.ptModel.input_is_BGR:
                batch = batch[:,:,:,::-1]  # RGB -> BGR

            return batch

        # Encapsulates the work done by TensorFlow for a single batch.
        def handle_request(batch):
            y = self.model.predict(batch)
            tf_out = [i.flatten() for i in y]
            return tf_out

        # Copies the output from TensorFlow into the SArrayBuilder and emits
        # progress.
        def consume_response(tf_out):
            sa = tc.SArray(tf_out, dtype=array)
            state['out'] = state['out'].append(sa)

            state['num_processed'] += len(tf_out)
            if verbose:
                print('Completed {num_processed:{width}d}/{total:{width}d}'.format(
                    width = len(str(state['total'])), **state))



        # These two arrays will swap off to avoid unnecessary allocations.
        state['batch_store'] = []
        def get_batch_array():
            batch_store = state['batch_store']

            if not batch_store:
                batch_store.append(np.zeros((batch_size,) + self.ptModel.input_image_shape, dtype=np.float32))

            return batch_store.pop()

        def batch_array_done(b):
            state['batch_store'].append(b)


        # Seed the iteration
        batch_info = next_batch(get_batch_array())

        # Iterate through the image batches, converting them into batches
        # of feature vectors.  Do the
        while batch_info is not None:

            # Get the now ready batch to process
            batch = ready_batch(batch_info)

            # Start the next one in the background.
            # Returns None if done.
            batch_info = next_batch(get_batch_array())

            # Now, process all this.
            predictions_from_tf = handle_request(batch)
            consume_response(predictions_from_tf)

            # Requeue the batch array now that we're done.
            batch_array_done(batch)

        # Now we have this compiled in
        return state['out']

    def get_coreml_model(self):
        import coremltools

        model_path = self.ptModel.get_model_path('coreml')
        return coremltools.models.MLModel(model_path)
