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
    from platform import system
    from ._internal_utils import _mac_ver
    from ._pre_trained_models import IMAGE_MODELS
    from turicreate import extensions

    # If we don't have Core ML, use a TensorFlow model.
    if system() != "Darwin" or _mac_ver() < (10, 13):
        ptModel = IMAGE_MODELS[model_name]()
        return TensorFlowFeatureExtractor(ptModel)

    download_path = _get_cache_dir()

    result = extensions.__dict__["image_deep_feature_extractor"]()
    result.init_options({"model_name": model_name, "download_path": download_path})
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

        model_path = ptModel.get_model_path("tensorflow")
        self.model = keras.models.load_model(model_path)

    def __del__(self):
        self.gpu_policy.stop()

    def extract_features(self, dataset, feature, batch_size=64, verbose=False):
        from array import array
        import turicreate as tc
        import numpy as np

        # Only expose the feature column to the SFrame-to-NumPy loader.
        image_sf = tc.SFrame({"image": dataset[feature]})

        # Encapsulate state in a dict to sidestep variable scoping issues.
        state = {}
        state["num_started"] = 0  # Images read from the SFrame
        state["num_processed"] = 0  # Images processed by TensorFlow
        state["total"] = len(dataset)  # Images in the dataset

        # We should be using SArrayBuilder, but it doesn't accept ndarray yet.
        # TODO: https://github.com/apple/turicreate/issues/2672
        # out = _tc.SArrayBuilder(dtype = array.array)
        state["out"] = tc.SArray(dtype=array)

        if verbose:
            print("Performing feature extraction on resized images...")

        # Provide an iterator-like interface around the SFrame.
        def has_next_batch():
            return state["num_started"] < state["total"]

        # Yield a numpy array representing one batch of images.
        def next_batch():
            # Compute the range of the SFrame to yield.
            start_index = state["num_started"]
            end_index = min(start_index + batch_size, state["total"])
            state["num_started"] = end_index

            # Allocate a numpy array with the desired shape.
            # TODO: Recycle the ndarray instances we're allocating below with
            # _np.zeros, instead of creating new ones every time.
            num_images = end_index - start_index
            shape = (num_images,) + self.ptModel.input_image_shape
            batch = np.zeros(shape, dtype=np.float32)

            # Resize and load the images.
            tc.extensions.sframe_load_to_numpy(
                image_sf,
                batch.ctypes.data,
                batch.strides,
                batch.shape,
                start_index,
                end_index,
            )

            # TODO: Converge to NCHW everywhere.
            batch = batch.transpose(0, 2, 3, 1)  # NCHW -> NHWC

            if self.ptModel.input_is_BGR:
                batch = batch[:, :, :, ::-1]  # RGB -> BGR

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
            state["out"] = state["out"].append(sa)

            state["num_processed"] += len(tf_out)
            if verbose:
                print(
                    "Completed {num_processed:{width}d}/{total:{width}d}".format(
                        width=len(str(state["total"])), **state
                    )
                )

        # Just iterate through the image batches, converting them into batches
        # of feature vectors. Note: the helper functions defined above are
        # designed to support a multi-threaded approach, allowing one Python
        # thread to drive TensorFlow computation and another to drive SFrame
        # traversal and image resizing. But test failures reveal an interaction
        # between the multi-threaded approach we used for MXNet and our usage of
        # TensorFlow, which we need to resolve before switching to the pipelined
        # implementation below.
        while has_next_batch():
            images_in_numpy = next_batch()
            predictions_from_tf = handle_request(images_in_numpy)
            consume_response(predictions_from_tf)

        # # Create a dedicated thread for performing TensorFlow work, using two
        # # FIFO queues for communication back and forth with this thread, with
        # # the goal of keeping TensorFlow busy throughout.
        # request_queue = _Queue()
        # response_queue = _Queue()
        # def tf_worker():
        #     from tensorflow import keras
        #     model_path = self.ptModel.get_model_path('tensorflow')
        #     self.model = keras.models.load_model(model_path)
        #
        #     while True:
        #         batch = request_queue.get()  # Consume request
        #         if batch is None:
        #             # No more work remains. Allow the thread to finish.
        #             return
        #         response_queue.put(handle_request(batch))  # Produce response
        # tf_worker_thread = _Thread(target=tf_worker)
        # tf_worker_thread.start()

        # try:
        #     # Attempt to have two requests in progress at any one time (double
        #     # buffering), so that the iterator is creating one batch while
        #     # TensorFlow performs inference on the other.
        #     if has_next_batch():
        #         request_queue.put(next_batch())  # Produce request
        #         while has_next_batch():
        #             request_queue.put(next_batch())  # Produce request
        #             consume_response(response_queue.get())
        #         consume_response(response_queue.get())
        # finally:
        #     # Tell the worker thread to shut down.
        #     request_queue.put(None)

        return state['out']

    def get_coreml_model(self):
        import coremltools

        model_path = self.ptModel.get_model_path("coreml")
        return coremltools.models.MLModel(model_path)
