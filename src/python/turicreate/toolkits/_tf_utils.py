# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import numpy as np
from turicreate._deps.minimal_package import _minimal_package_import_check


class TensorFlowGPUPolicy(object):
    """
    Suppresses GPU usage within TensorFlow if the Turi Create configuration has
    requested no GPUs to be used. Can also be used as a context manager.
    """

    def start(self):
        import turicreate as tc

        self.force_cpu = tc.config.get_num_gpus() == 0
        if self.force_cpu:
            # Setting the environment variable CUDA_VISIBLE_DEVICES appears to
            # be the most reliable way to suppress GPU usage. Set that, but
            # first save the old value so we can restore it when we're done.
            import os

            if "CUDA_VISIBLE_DEVICES" in os.environ:
                self.cuda_visible_devices = os.environ["CUDA_VISIBLE_DEVICES"]
            else:
                self.cuda_visible_devices = None
            os.environ["CUDA_VISIBLE_DEVICES"] = ""

    def stop(self):
        if self.force_cpu:
            import os

            if self.cuda_visible_devices is not None:
                os.environ["CUDA_VISIBLE_DEVICES"] = self.cuda_visible_devices
            elif "CUDA_VISIBLE_DEVICES" in os.environ:
                del os.environ["CUDA_VISIBLE_DEVICES"]

    def __enter__(self):
        self.start()

    def __exit__(self, exception_type, exception_val, exception_traceback):
        self.stop()


def suppress_tensorflow_warnings():
    """
    Suppresses tensorflow warnings
    """
    import os

    os.environ["TF_CPP_MIN_LOG_LEVEL"] = "1"

    _tf = _minimal_package_import_check("tensorflow.compat.v1")

    _tf.logging.set_verbosity(_tf.logging.ERROR)
    _tf.debugging.set_log_device_placement(False)


def is_gpu_available():
    tf = _minimal_package_import_check("tensorflow")

    tf_minor_version = int(tf.version.VERSION.split(".")[1])

    if tf_minor_version == 0:
        gpu_names = tf.config.experimental.list_physical_devices("GPU")
    else:
        gpu_names = tf.config.list_physical_devices("GPU")

    return len(gpu_names) > 0


def convert_shared_float_array_to_numpy(array):
    """
    The initialization from C++ implementation is mapped to SharedFloatArray
    in Python through Pybind. It must be converted to numpy arrays to be used
    in TensorFlow.

    Parameters
    ----------
    array: SharedFloatArray

    Returns
    -------
    return: Numpy Array
    SharedFloatArray casted as Numpy Array

    """
    return np.array(array, copy=False, dtype=np.float32)


def convert_conv1d_coreml_to_tf(conv_weights):
    """
    The Convolutional weights in CoreML specification converted to
    the TensorFlow format for training in TensorFlow.

    Parameters
    ----------
    conv_weights: 4d numpy array of shape
       [outputChannels, kernelChannels, kernelHeight, kernelWidth]

    Returns
    -------
    return: 3d numpy array of shape
       [kernelWidth, kernelChannels, outputChannels]
       since kernelHeight = 1 for conv1d

    """

    conv_weights = np.transpose(conv_weights, (3, 1, 0, 2))
    return np.squeeze(conv_weights, axis=3)


def convert_conv2d_coreml_to_tf(conv_weights):

    """
    The Convolutional weights in CoreML specification converted to
    the TensorFlow format for training in TensorFlow.

    Parameters
    ----------
    conv_weights: 4d numpy array of shape
       [output_channels, input_channels, filter_height, filter_width]

    Returns
    -------
    return: 4d numpy array of shape
       [filter_height, filter_width, input_channels, output_channels]

    """
    conv_weights = np.transpose(conv_weights, (2, 3, 1, 0))
    return conv_weights


def convert_dense_coreml_to_tf(dense_weights):
    """
    The Dense layer weights from CoreML are [C_out, C_in, 1, 1] and need to be
    converted to [C_in, C_out] for TensorFlow.

    Parameters
    ----------
    dense_weights: 4d numpy array of shape
       [outputChannels, inChannels, 1, 1]

    Returns
    -------
    return: 2d numpy array of shape
       [outputChannels, kernelChannels, kernelHeight, kernelWidth]

    """
    dense_weights = np.transpose(dense_weights, (1, 0, 2, 3))
    return np.reshape(dense_weights, (dense_weights.shape[0], dense_weights.shape[1]))


def convert_conv1d_tf_to_coreml(conv_weights):
    """
    Convolutional weights from TensorFlow in the format [kernelWidth, kernelChannels, outputChannels]
    are converted back in CoreML specifications [outputChannels, kernelChannels, kernelHeight, kernelWidth].

    Parameters
    ----------
    conv_weights: 3d numpy array of shape
       [kernelWidth, kernelChannels, outputChannels]
       since kernelHeight = 1 for conv1d

    Returns
    -------
    return: 4d numpy array of shape
       [outputChannels, kernelChannels, kernelHeight, kernelWidth]

    """
    conv_weights = np.expand_dims(conv_weights, axis=2)
    conv_weights = np.transpose(conv_weights, (3, 1, 2, 0))
    return conv_weights


def convert_conv2d_tf_to_coreml(conv_weights):
    """
    Convolutional weights from TensorFlow in the format [filter_height, filter_width, input_channels, output_channels]
    are converted back in CoreML specifications output_channels, input_channels, filter_height, filter_width].

    Parameters
    ----------
    conv_weights: 4d numpy array of shape
       [filter_height, filter_width, input_channels, output_channels]
    Returns
    -------
    return: 4d numpy array of shape
       [output_channels, input_channels, filter_height, filter_width]

    """
    conv_weights = np.transpose(conv_weights, (3, 2, 0, 1))
    return conv_weights

def convert_dense_tf_to_coreml(dense_weights):
    """
    The Dense layer weights from TensorFlow are converted
    back to CoreML specification.

    Parameters
    ----------
    dense_weights: 2d numpy array of shape
       [outputChannels, kernelChannels, kernelHeight, kernelWidth]

    Returns
    -------
    return: 4d numpy array of shape
       [outputChannels, inChannels, 1, 1]

    """
    dense_weights = np.transpose(dense_weights)
    return np.reshape(
        dense_weights, (dense_weights.shape[0], dense_weights.shape[1], 1, 1)
    )
