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
        """
        Parameters
        ----------
        dataset: SFrame
            SFrame of images
        """
        from ._mxnet._mx_sframe_iter import SFrameImageIter as _SFrameImageIter
        from six.moves.queue import Queue as _Queue
        from threading import Thread as _Thread
        import turicreate as _tc
        import array

        if len(dataset) == 0:
            return _tc.SArray([], array.array)

        batch_size = min(len(dataset), batch_size)
        # Make a data iterator
        dataIter = _SFrameImageIter(sframe=dataset, data_field=[feature], batch_size=batch_size, image_shape=self.image_shape)

        # Setup the MXNet model
        model = MXFeatureExtractor._get_mx_module(self.ptModel.mxmodel,
                self.data_layer, self.feature_layer, self.context, self.image_shape, batch_size)

        out = _tc.SArrayBuilder(dtype = array.array)
        progress = { 'num_processed' : 0, 'total' : len(dataset) }
        if verbose:
            print("Performing feature extraction on resized images...")

        # Encapsulates the work done by the MXNet model for a single batch
        def handle_request(batch):
            model.forward(batch)
            mx_out = [array.array('d',m) for m in model.get_outputs()[0].asnumpy()]
            if batch.pad != 0:
                # If batch size is not evenly divisible by the length, it will loop back around.
                # We don't want that.
                mx_out = mx_out[:-batch.pad]
            return mx_out

        # Copies the output from MXNet into the SArrayBuilder and emits progress
        def consume_response(mx_out):
            out.append_multiple(mx_out)

            progress['num_processed'] += len(mx_out)
            if verbose:
                print('Completed {num_processed:{width}d}/{total:{width}d}'.format(
                    width = len(str(progress['total'])), **progress))

        # Create a dedicated thread for performing MXNet work, using two FIFO
        # queues for communication back and forth with this thread, with the
        # goal of keeping MXNet busy throughout.
        request_queue = _Queue()
        response_queue = _Queue()
        def mx_worker():
            while True:
                batch = request_queue.get()  # Consume request
                if batch is None:
                    # No more work remains. Allow the thread to finish.
                    return
                response_queue.put(handle_request(batch))  # Produce response
        mx_worker_thread = _Thread(target=mx_worker)
        mx_worker_thread.start()

        try:
            # Attempt to have two requests in progress at any one time (double
            # buffering), so that the iterator is creating one batch while MXNet
            # performs inference on the other.
            if dataIter.has_next:
                request_queue.put(next(dataIter))  # Produce request
                while dataIter.has_next:
                    request_queue.put(next(dataIter))  # Produce request
                    consume_response(response_queue.get())
                consume_response(response_queue.get())
        finally:
            # Tell the worker thread to shut down.
            request_queue.put(None)

        return out.close()

    def get_coreml_model(self):
        import coremltools

        model_path = self.ptModel.get_model_path('coreml')
        return coremltools.models.MLModel(model_path)
