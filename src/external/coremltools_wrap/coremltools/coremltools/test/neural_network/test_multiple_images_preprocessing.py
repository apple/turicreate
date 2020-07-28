from __future__ import print_function as _

import json
import os
import shutil
import subprocess
import tarfile
import tempfile
import unittest
from subprocess import Popen

import PIL.Image
import numpy as np
import pytest

import coremltools
from coremltools._deps import _HAS_KERAS2_TF
from coremltools.models.utils import _macos_version, _is_macos

if _HAS_KERAS2_TF:
    import keras
    from keras.models import Sequential, Model
    from keras.layers import Activation, GlobalMaxPooling2D, Input

FOLDER_NAME = "multiple_images_preprocessing"


def extract_tarfile(input_filename, dest_dir):
    with tarfile.open(input_filename, "r:gz") as tar:
        tar.extractall(dest_dir)


def load_mlmodel(model_path):
    load_args = [" /usr/local/bin/coremltest", "load", "-modelPath", model_path]
    print("Loading {}".format(model_path))
    process = Popen(
        (" ").join(load_args),
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        shell=True,
    )
    stdout, err = process.communicate()

    if not err:
        return True
    else:
        print(" The error is {}".format(err.decode()))
        return False


def compare_models(caffe_preds, coreml_preds):
    max_relative_error = 0
    for i in range(len(coreml_preds)):
        max_den = max(1.0, np.abs(caffe_preds[i]), np.abs(coreml_preds[i]))
        relative_error = np.abs(caffe_preds[i] / max_den - coreml_preds[i] / max_den)
        if relative_error > max_relative_error:
            max_relative_error = relative_error

    print("maximum relative error: ", max_relative_error)
    return max_relative_error


@unittest.skipIf(not _HAS_KERAS2_TF, "Missing keras. Skipping tests.")
@pytest.mark.keras2
class ManyImagesKeras(unittest.TestCase):
    def test_keras_1_image_bias(self):
        # define Keras model and get prediction
        input_shape = (100, 50, 3)
        model = Sequential()
        model.add(Activation("linear", input_shape=input_shape))

        data = np.ones(input_shape)
        keras_input = np.ones(input_shape)
        data[:, :, 0] = 128.0
        data[:, :, 1] = 27.0
        data[:, :, 2] = 200.0
        red_bias = -12.0
        green_bias = -20
        blue_bias = -4
        keras_input[:, :, 0] = data[:, :, 0] + red_bias
        keras_input[:, :, 1] = data[:, :, 1] + green_bias
        keras_input[:, :, 2] = data[:, :, 2] + blue_bias

        keras_preds = model.predict(np.expand_dims(keras_input, axis=0))
        keras_preds = np.transpose(keras_preds, [0, 3, 1, 2]).flatten()

        # convert to coreml and get predictions
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, "keras.mlmodel")
        from coremltools.converters import keras as keras_converter

        coreml_model = keras_converter.convert(
            model,
            input_names=["data"],
            output_names=["output"],
            image_input_names=["data"],
            red_bias=red_bias,
            green_bias=green_bias,
            blue_bias=blue_bias,
        )

        if _is_macos() and _macos_version() >= (10, 13):
            coreml_input_dict = dict()
            coreml_input_dict["data"] = PIL.Image.fromarray(data.astype(np.uint8))
            coreml_preds = coreml_model.predict(coreml_input_dict)["output"].flatten()

            self.assertEquals(len(keras_preds), len(coreml_preds))
            max_relative_error = compare_models(keras_preds, coreml_preds)
            self.assertAlmostEquals(max(max_relative_error, 0.001), 0.001, delta=1e-6)

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)

    def test_keras_2_image_bias(self):
        # define Keras model and get prediction
        input_shape1 = (100, 60, 3)
        input_shape2 = (23, 45, 3)

        data1 = Input(shape=input_shape1)
        data2 = Input(shape=input_shape2)
        a_pool = GlobalMaxPooling2D()(data1)
        b_pool = GlobalMaxPooling2D()(data2)
        output = keras.layers.add([a_pool, b_pool])
        model = Model(inputs=[data1, data2], outputs=output)

        data1 = np.ones(input_shape1)
        data2 = np.ones(input_shape2)
        keras_input1 = np.ones(input_shape1)
        keras_input2 = np.ones(input_shape2)

        data1[:, :, 0] = 100.0
        data1[:, :, 1] = 79.0
        data1[:, :, 2] = 194.0

        data2[:, :, 0] = 130.0
        data2[:, :, 1] = 91.0
        data2[:, :, 2] = 11.0

        red_bias1 = -88.0
        green_bias1 = -2
        blue_bias1 = -40

        red_bias2 = -100.0
        green_bias2 = -29
        blue_bias2 = -15

        keras_input1[:, :, 0] = data1[:, :, 2] + blue_bias1
        keras_input1[:, :, 1] = data1[:, :, 1] + green_bias1
        keras_input1[:, :, 2] = data1[:, :, 0] + red_bias1

        keras_input2[:, :, 0] = data2[:, :, 0] + red_bias2
        keras_input2[:, :, 1] = data2[:, :, 1] + green_bias2
        keras_input2[:, :, 2] = data2[:, :, 2] + blue_bias2

        keras_preds = model.predict(
            [np.expand_dims(keras_input1, axis=0), np.expand_dims(keras_input2, axis=0)]
        )
        keras_preds = keras_preds.flatten()

        # convert to coreml and get predictions
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, "keras.mlmodel")
        from coremltools.converters import keras as keras_converter

        coreml_model = keras_converter.convert(
            model,
            input_names=["data1", "data2"],
            output_names=["output"],
            image_input_names=["data1", "data2"],
            red_bias={"data1": red_bias1, "data2": red_bias2},
            green_bias={"data1": green_bias1, "data2": green_bias2},
            blue_bias={"data1": blue_bias1, "data2": blue_bias2},
            is_bgr={"data1": True, "data2": False},
        )

        if _is_macos() and _macos_version() >= (10, 13):
            coreml_input_dict = dict()
            coreml_input_dict["data1"] = PIL.Image.fromarray(data1.astype(np.uint8))
            coreml_input_dict["data2"] = PIL.Image.fromarray(data2.astype(np.uint8))
            coreml_preds = coreml_model.predict(coreml_input_dict)["output"].flatten()

            # compare
            self.assertEquals(len(keras_preds), len(coreml_preds))
            max_relative_error = compare_models(keras_preds, coreml_preds)
            self.assertAlmostEquals(max(max_relative_error, 0.001), 0.001, delta=1e-6)

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)
