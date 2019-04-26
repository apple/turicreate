from __future__ import print_function
import unittest
import tarfile
import shutil
import json
import os
import tempfile
from subprocess import Popen
import subprocess
import numpy as np
from coremltools.converters import caffe as caffe_converter
from coremltools.models.utils import macos_version
import coremltools
import PIL.Image
import pytest

from coremltools._deps import HAS_KERAS2_TF

if HAS_KERAS2_TF:
    import keras
    from keras.models import Sequential, Model
    from keras.layers import Activation, GlobalMaxPooling2D, Input
    from coremltools.converters import keras as kerasConverter

try:
    nets_path = os.environ["CAFFE_MODELS_PATH"]
    nets_path = nets_path + '/'
except:
    nets_path = None

FOLDER_NAME = 'multiple_images_preprocessing'


def extract_tarfile(input_filename, dest_dir):
    with tarfile.open(input_filename, "r:gz") as tar:
        tar.extractall(dest_dir)


def load_mlmodel(model_path):
    load_args = [' /usr/local/bin/coremltest', 'load', '-modelPath', model_path]
    print('Loading {}'.format(model_path))
    process = Popen((" ").join(load_args),
                    stdin=subprocess.PIPE,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    shell=True)
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
        relative_error = np.abs(caffe_preds[i]/max_den - coreml_preds[i]/max_den)
        if relative_error > max_relative_error:
            max_relative_error = relative_error

    print('maximum relative error: ', max_relative_error)
    #print('caffe preds : ', caffe_preds)
    #print('coreml preds: ', coreml_preds)
    return max_relative_error

@unittest.skipIf(nets_path is None, "Unable to find CAFFE_MODELS_PATH")
class ManyImages(unittest.TestCase):
    """
    Unit test case for caffe layers
    """
    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading common utilities.
        """

    def _evaluate_and_test_model_meanpreprocessing(self, n):

        failed_tests_load = []
        failed_tests_conversion = []
        failed_tests_evaluation = []

        extract_tarfile('{}nets/{}.gz'.format(nets_path, FOLDER_NAME), '{}nets/'.format(nets_path))

        path_prototxt = '{}nets/{}/{}/image{}.prototxt'.format(nets_path, FOLDER_NAME, str(n), str(n))
        path_caffemodel = '{}nets/{}/{}/image{}.caffemodel'.format(nets_path, FOLDER_NAME, str(n), str(n))
        path_mlmodel = '{}nets/{}/{}/image{}.mlmodel'.format(nets_path, FOLDER_NAME, str(n), str(n))
        if n == 1:
            path_binaryproto = '{}nets/{}/1/mean_binary_proto1.binaryproto'.format(nets_path, FOLDER_NAME)
        else:
            path_binaryproto = dict()
            for i in range(n):
                path_binaryproto["data{}".format(str(i+1))] = '{}nets/{}/{}/mean_binary_proto{}.binaryproto'.format(nets_path, FOLDER_NAME, str(n), str(i+1))

        image_input_names = []
        for i in range(n):
            image_input_names.append("data{}".format(str(i+1)))

        #convert it
        try:
            model = caffe_converter.convert((path_caffemodel, path_prototxt, path_binaryproto), image_input_names = image_input_names)
            model.save(path_mlmodel)
        except RuntimeError as e:
            print(e)
            failed_tests_conversion.append('image mean preprocessing: conversion failure')

        #load it (compile it)
        load_result = load_mlmodel(path_mlmodel)
        if load_result is False:
            failed_tests_load.append('image mean preprocessing: load failure')


        #load Caffe's input and output
        with open('{}nets/{}/{}/input.json'.format(nets_path, FOLDER_NAME, str(n))) as data_file:
            input_data_dict = json.load(data_file)
        with open('{}nets/{}/{}/output.json'.format(nets_path, FOLDER_NAME, str(n))) as data_file:
            output_data_dict = json.load(data_file)

        output_data = np.array(output_data_dict["output_data"])

        coreml_input_dict = dict()

        for i in range(n):
            input_data = np.array(input_data_dict["input_data{}".format(str(i+1))]).astype(np.uint8)
            img = PIL.Image.fromarray(np.transpose(input_data[0,:,:,:],[1,2,0]))
            coreml_input_dict["data{}".format(str(i+1))] = img

        #load and evaluate mlmodel
        mlmodel = coremltools.models.MLModel(path_mlmodel)
        if macos_version() >= (10, 13):
            coreml_out = mlmodel.predict(coreml_input_dict)['output']

            caffe_preds = output_data.flatten()
            coreml_preds = coreml_out.flatten()
            if len(caffe_preds) != len(coreml_preds):
                failed_tests_evaluation.append('single image mean preprocessing: evaluation failure')

            max_relative_error = compare_models(output_data.flatten(), coreml_out.flatten())
            if max_relative_error > 0.001:
                failed_tests_evaluation.append('single image mean preprocessing: evaluation failure')

            self.assertEqual(failed_tests_conversion,[])
            self.assertEqual(failed_tests_load,[])
            self.assertEqual(failed_tests_evaluation,[])
        shutil.rmtree('{}nets/{}'.format(nets_path, FOLDER_NAME))


    def _evaluate_and_test_model_biasprocessing(self, n, red_bias, green_bias, blue_bias, image_scale, is_bgr):

        failed_tests_load = []
        failed_tests_conversion = []
        failed_tests_evaluation = []

        extract_tarfile('{}nets/{}.gz'.format(nets_path, FOLDER_NAME), '{}nets/'.format(nets_path))

        path_prototxt = '{}nets/{}/{}_bias/image{}.prototxt'.format(nets_path, FOLDER_NAME, str(n), str(n))
        path_caffemodel = '{}nets/{}/{}_bias/image{}.caffemodel'.format(nets_path, FOLDER_NAME, str(n), str(n))
        path_mlmodel = '{}nets/{}/{}_bias/image{}.mlmodel'.format(nets_path, FOLDER_NAME, str(n), str(n))

        image_input_names = []
        for i in range(n):
            image_input_names.append("data{}".format(str(i+1)))

        #convert it
        try:
            model = caffe_converter.convert(model = (path_caffemodel, path_prototxt), image_input_names = image_input_names,
                                            red_bias = red_bias, green_bias = green_bias, blue_bias = blue_bias,
                                            image_scale = image_scale, is_bgr = is_bgr)
            model.save(path_mlmodel)
        except RuntimeError as e:
            print(e)
            failed_tests_conversion.append('image bias preprocessing: conversion failure')

        #load it (compile it)
        load_result = load_mlmodel(path_mlmodel)
        if load_result is False:
            failed_tests_load.append('image bias preprocessing: load failure')

        if macos_version() >= (10, 13):
            #load Caffe's input and output
            with open('{}nets/{}/{}_bias/input.json'.format(nets_path, FOLDER_NAME, str(n))) as data_file:
                input_data_dict = json.load(data_file)
            with open('{}nets/{}/{}_bias/output.json'.format(nets_path, FOLDER_NAME, str(n))) as data_file:
                output_data_dict = json.load(data_file)

            output_data = np.array(output_data_dict["output_data"])

            coreml_input_dict = dict()

            for i in range(n):
                input_data = np.array(input_data_dict["input_data{}".format(str(i+1))]).astype(np.uint8)
                img = PIL.Image.fromarray(np.transpose(input_data[0,:,:,:],[1,2,0]))
                coreml_input_dict["data{}".format(str(i+1))] = img

            #load and evaluate mlmodel
            mlmodel = coremltools.models.MLModel(path_mlmodel)
            coreml_out = mlmodel.predict(coreml_input_dict)['output']

            caffe_preds = output_data.flatten()
            coreml_preds = coreml_out.flatten()
            if len(caffe_preds) != len(coreml_preds):
                failed_tests_evaluation.append('single image bias preprocessing: evaluation failure')

            max_relative_error = compare_models(output_data.flatten(), coreml_out.flatten())
            if max_relative_error > 0.001:
                failed_tests_evaluation.append('single image bias preprocessing: evaluation failure')

            self.assertEqual(failed_tests_conversion,[])
            self.assertEqual(failed_tests_load,[])
            self.assertEqual(failed_tests_evaluation,[])

        shutil.rmtree('{}nets/{}'.format(nets_path, FOLDER_NAME))

    def test_1_mean_image(self):
        self._evaluate_and_test_model_meanpreprocessing(1)

    def test_2_mean_images(self):
        self._evaluate_and_test_model_meanpreprocessing(2)

    def test_3_mean_images(self):
        self._evaluate_and_test_model_meanpreprocessing(3)

    def test_1_image_bias(self):
        self._evaluate_and_test_model_biasprocessing(n=1,
                                                    red_bias = -34, green_bias = -123,
                                                    blue_bias = -22, image_scale = 0.75, is_bgr = False)

    def test_2_image_bias(self):
        self._evaluate_and_test_model_biasprocessing(n=2,
                                                    red_bias = {"data1":-11, "data2":-87},
                                                    green_bias = {"data1":-56, "data2":-78},
                                                    blue_bias = {"data1":-122, "data2":-76},
                                                    image_scale = {"data1":0.5, "data2":0.4},
                                                    is_bgr = {"data1":False, "data2":True})



@unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras. Skipping tests.')
@pytest.mark.keras2
class ManyImagesKeras(unittest.TestCase):

    def test_keras_1_image_bias(self):

        #define Keras model and get prediction
        input_shape=(100,50,3)
        model = Sequential()
        model.add(Activation('linear', input_shape=input_shape))

        data = np.ones(input_shape)
        keras_input = np.ones(input_shape)
        data[:,:,0] = 128.0;
        data[:,:,1] = 27.0;
        data[:,:,2] = 200.0;
        red_bias = -12.0;
        green_bias = -20;
        blue_bias = -4;
        keras_input[:,:,0] = data[:,:,0] + red_bias;
        keras_input[:,:,1] = data[:,:,1] + green_bias;
        keras_input[:,:,2] = data[:,:,2] + blue_bias;

        keras_preds = model.predict(np.expand_dims(keras_input, axis = 0))
        keras_preds = np.transpose(keras_preds, [0,3,1,2]).flatten()

        #convert to coreml and get predictions
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'keras.mlmodel')
        from coremltools.converters import keras as keras_converter
        coreml_model = keras_converter.convert(model, input_names = ['data'], output_names = ['output'],
                                                image_input_names = ['data'],
                                                red_bias = red_bias,
                                                green_bias = green_bias,
                                                blue_bias = blue_bias)
        #coreml_model.save(model_path)
        #coreml_model = coremltools.models.MLModel(model_path)

        if macos_version() >= (10, 13):
            coreml_input_dict = dict()
            coreml_input_dict["data"] = PIL.Image.fromarray(data.astype(np.uint8))
            coreml_preds = coreml_model.predict(coreml_input_dict)['output'].flatten()

            self.assertEquals(len(keras_preds), len(coreml_preds))
            max_relative_error = compare_models(keras_preds, coreml_preds)
            self.assertAlmostEquals(max(max_relative_error, .001), .001, delta = 1e-6)


        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)


    def test_keras_2_image_bias(self):

        #define Keras model and get prediction
        input_shape1 = (100,60,3)
        input_shape2 = (23,45,3)

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

        data1[:,:,0] = 100.0
        data1[:,:,1] = 79.0
        data1[:,:,2] = 194.0

        data2[:,:,0] = 130.0
        data2[:,:,1] = 91.0
        data2[:,:,2] = 11.0


        red_bias1 = -88.0;
        green_bias1 = -2;
        blue_bias1 = -40;

        red_bias2 = -100.0;
        green_bias2 = -29;
        blue_bias2 = -15;

        keras_input1[:,:,0] = data1[:,:,2] + blue_bias1;
        keras_input1[:,:,1] = data1[:,:,1] + green_bias1;
        keras_input1[:,:,2] = data1[:,:,0] + red_bias1;

        keras_input2[:,:,0] = data2[:,:,0] + red_bias2;
        keras_input2[:,:,1] = data2[:,:,1] + green_bias2;
        keras_input2[:,:,2] = data2[:,:,2] + blue_bias2;

        keras_preds = model.predict([np.expand_dims(keras_input1, axis = 0), np.expand_dims(keras_input2, axis = 0)])
        keras_preds = keras_preds.flatten()

        #convert to coreml and get predictions
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'keras.mlmodel')
        from coremltools.converters import keras as keras_converter
        coreml_model = keras_converter.convert(model, input_names = ['data1', 'data2'], output_names = ['output'],
                                                image_input_names = ['data1', 'data2'],
                                                red_bias = {'data1': red_bias1, 'data2': red_bias2},
                                                green_bias = {'data1': green_bias1, 'data2': green_bias2},
                                                blue_bias = {'data1': blue_bias1, 'data2': blue_bias2},
                                                is_bgr = {'data1': True, 'data2': False})
        #coreml_model.save(model_path)
        #coreml_model = coremltools.models.MLModel(model_path)

        if macos_version() >= (10, 13):
            coreml_input_dict = dict()
            coreml_input_dict["data1"] = PIL.Image.fromarray(data1.astype(np.uint8))
            coreml_input_dict["data2"] = PIL.Image.fromarray(data2.astype(np.uint8))
            coreml_preds = coreml_model.predict(coreml_input_dict)['output'].flatten()

            #compare
            self.assertEquals(len(keras_preds), len(coreml_preds))
            max_relative_error = compare_models(keras_preds, coreml_preds)
            self.assertAlmostEquals(max(max_relative_error, .001), .001, delta = 1e-6)

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)
