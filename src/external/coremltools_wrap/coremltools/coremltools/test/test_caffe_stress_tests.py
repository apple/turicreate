import unittest
import os
from subprocess import Popen
import subprocess
import tarfile
import shutil
import json
import numpy as np
import math
from coremltools.converters import caffe as caffe_converter
from coremltools.models.utils import macos_version
nets_path = os.getenv('CAFFE_MODELS_PATH', '')
nets_path = nets_path + '/' if nets_path else ''
import coremltools
import pytest


def extract_tarfile(input_filename, dest_dir):
    with tarfile.open(input_filename, "r:gz") as tar:
        tar.extractall(dest_dir)


def traverse_caffe_nets(layer_type):
    for root, dirs, files in os.walk(
            '{}nets/{}/caffemodels/'.format(nets_path, layer_type)):
        return files


def traverse_data_files(layer_type):
    for root, dirs, files in os.walk(
            '{}nets/{}/data/'.format(nets_path, layer_type)):
        return set(files)

"""def convert(model, image_input_names=[], is_bgr=False,
            red_bias=0.0, blue_bias=0.0, green_bias=0.0, gray_bias=0.0,
            image_scale=1.0, class_labels=None, predicted_feature_name=None):
            """
def conversion_to_mlmodel(net_name, proto_name, layer_type, input_layer):
    filename= '{}nets/{}/mlkitmodels/{}.mlmodel'.format(
                              nets_path, layer_type, net_name)
    caffe_model_path = '{}nets/{}/caffemodels/{}.caffemodel'.format(
                              nets_path, layer_type, net_name),
    proto_path = '{}nets/{}/prototxt/{}.prototxt'.format(
                   nets_path, layer_type, proto_name)
    model_path = caffe_model_path[0]
    if isinstance(input_layer, str):
        input_layer = [input_layer]
    try:
        model = caffe_converter.convert(
            (model_path, proto_path),
        )
        model.save(filename)
    except RuntimeError as e:
        print(e)
        return False
    return True


def load_mlmodel(net_name, layer_type):
    load_args = [' /usr/local/bin/coremltest',
                 'load',
                 '-modelPath',
                 '{}nets/{}/mlkitmodels/{}.mlmodel'.format(
                     nets_path, layer_type, net_name),
                 ]
    print('Loading {}'.format(net_name))
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


def evaluation_data(net_name, layer_type, data_files):
    if 'input' in net_name:
        for file in data_files:
            if 'output' in file:
                output_data_file = file
            elif 'input' in file:
                input_data_file = file
            else:
                return False

        with open('{}nets/{}/data/{}'.format(
                nets_path, layer_type, input_data_file)
        ) as data_file:
            input_net_data = json.load(data_file)
        with open('{}nets/{}/data/{}'.format(
                nets_path, layer_type, output_data_file)
        ) as data_file:
            output_net_data = json.load(data_file)
        if isinstance(input_net_data, list):
            input_data = []
            for data in input_net_data:
                input_data.append(np.array(data["input_data"]))
        else:
            input_data = np.array(input_net_data["input_data"], dtype = 'f')
        if isinstance(output_net_data, list):
            output_data = []
            for data in output_net_data:
                output_data.append(np.array(data["output_data"]))
        else:
            output_data = np.array(output_net_data["output_data"])

        return input_data, output_data


@unittest.skipIf(not nets_path, 'Caffe nets path environment variable not '
                                'found. Skipping all caffe nose tests')
class CaffeLayers(unittest.TestCase):
    """
    Unit test case for caffe layers
    """
    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading common utilities.
        """

    def run_case(self, layer_type, input_layer, output_layer, delta=1e-2):
        self.maxDiff = None
        extract_tarfile('{}nets/{}.gz'.format(nets_path, layer_type),
                        '{}nets/'.format(nets_path))
        nets = traverse_caffe_nets(layer_type)
        data_files = traverse_data_files(layer_type)
        failed_tests_load = []
        failed_tests_conversion = []
        failed_tests_evaluation = []
        counter = 0
        for net_name_proto in nets:

            counter += 1
            net_data_files = []
            proto_name = \
                net_name_proto.split("_")[0] + \
                "_" + \
                net_name_proto.split("_")[1]
            for file in data_files:
                if proto_name + '_' in file:
                    net_data_files.append(file)
            net_name = net_name_proto.split(".")[0]
            conversion_result = conversion_to_mlmodel(
                net_name,
                proto_name,
                layer_type,
                input_layer
            )
            if macos_version() >= (10, 13):
                if conversion_result is False:
                    failed_tests_conversion.append(net_name)
                    continue
                load_result = load_mlmodel(net_name, layer_type)
                if load_result is False:
                    failed_tests_load.append(net_name)
                if 'input' in net_name:
                    evaluation_result, failed_tests_evaluation = \
                        self.evaluate_model(
                            net_name,
                            layer_type,
                            input_layer,
                            output_layer,
                            net_data_files,
                            failed_tests_evaluation,
                            counter,
                            delta)
        with open('./failed_tests_{}.json'.format(layer_type), mode='w') \
                as file:
            json.dump({'conversion': failed_tests_conversion,
                       'load': failed_tests_load,
                       'evaluation': failed_tests_evaluation},
                      file)

        self.assertEqual(failed_tests_conversion,
                         [])
        self.assertEqual(failed_tests_load,
                         [])
        self.assertEqual(failed_tests_evaluation,
                         [])
        shutil.rmtree('{}nets/{}'.format(nets_path, layer_type))

    def evaluate_model(self,
                       net_name,
                       layer_type,
                       input_layer,
                       output_layer,
                       data_files,
                       failed_tests,
                       counter,
                       delta=1e-2):
        input_data, output_data = evaluation_data(
            net_name,
            layer_type,
            data_files)
        model_path = '{}nets/{}/mlkitmodels/{}.mlmodel'.format(
            nets_path,
            layer_type,
            net_name
        )
        coremlmodel = coremltools.models.MLModel(model_path)
        mlmodel_input = {}
        if isinstance(input_layer, list):
            i = 0
            for layer in input_layer:
                if len(input_data[i].shape) == 4:
                    if input_data[i].shape[0] > 1:
                        mlmodel_input[layer] = np.expand_dims(input_data[i], axis=0)
                    else:
                        mlmodel_input[layer] = input_data[i][0,:,:,:]
                else:
                    mlmodel_input[layer] = input_data[i]
                i += 1
        else:
            if len(input_data.shape) == 4:
                if input_data.shape[0] > 1:
                    if str(coremlmodel.output_description).split('(')[1][:-1] == 'LayerEmbed':
                        mlmodel_input[input_layer] = np.expand_dims(input_data, axis=1)
                    else:
                        mlmodel_input[input_layer] = np.expand_dims(input_data, axis=0)
                else :
                    mlmodel_input[input_layer] = input_data[0,:,:,:]
            else:
                mlmodel_input[input_layer] = input_data[0]
        if isinstance(output_layer, list):
            output_preds = coremlmodel.predict(mlmodel_input)
            coreml_preds = []
            caffe_preds = []
            i = 0
            for key in sorted(output_preds):
                coreml_preds.extend(output_preds[key].flatten().tolist())
                caffe_preds.extend(output_data[i].flatten().tolist())
                i += 1
        else:
            output_layer_name = str(coremlmodel.output_description).split('(')[1].split(')')[0]
            coreml_preds = coremlmodel.predict(mlmodel_input)[output_layer_name].flatten()
            caffe_preds = output_data.flatten()

        if len(coreml_preds) != len(caffe_preds):
            failed_tests.append(net_name)
            return relative_error, failed_tests

        for i in range(len(caffe_preds)):
            max_den = max(1.0, np.abs(caffe_preds[i]), np.abs(coreml_preds[i]))
            relative_error = np.abs(caffe_preds[i]/max_den - coreml_preds[i]/max_den)
            if relative_error > delta and np.abs(caffe_preds[i]) < 1e10:
                failed_tests.append(net_name)
                break

        return relative_error, failed_tests

    @pytest.mark.slow
    def test_caffe_inner_product_layer(self):
        self.run_case(
            layer_type='inner_product',
            input_layer='data',
            output_layer='LayerInnerProduct'
        )

    @pytest.mark.slow
    def test_caffe_inner_product_activation_layer(self):
        self.run_case(
            layer_type='inner_product_activation',
            input_layer='data',
            output_layer='LayerActivation'
        )

    #@unittest.skip("Add Test cases where group is not 1: Radar: 32739970")
    @pytest.mark.slow
    def test_convolutional_layer(self):
        self.run_case(
            layer_type='convolutional',
            input_layer='data',
            output_layer='LayerConvolution'
        )

    #@unittest.skip("Add Test cases where group and dilation are not 1 Radar: 32739970")
    @pytest.mark.slow
    def test_deconvolution_layer(self):
        self.run_case(
            layer_type='deconvolution',
            input_layer='data',
            output_layer='LayerDeconvolution'
        )

    def test_reduction_layer(self):
        self.run_case(
            layer_type='reduction',
            input_layer='data',
            output_layer='LayerReduction'
            )

    def test_scale_layer(self):
        self.run_case(
            layer_type='scale',
            input_layer=['LayerInput1', 'LayerInput2'],
            output_layer='LayerScale'
        )

    def test_slice_layer(self):
        self.run_case(
            layer_type='slice',
            input_layer='data',
            output_layer=['LayerSlice', 'LayerSlice1', 'LayerSlice2']
        )

    def test_bias_layer(self):
        self.run_case(
            layer_type='bias',
            input_layer=['LayerInput1', 'LayerInput2'],
            output_layer='LayerBias'
        )

    @pytest.mark.slow
    def test_crop_layer(self):
        self.run_case(
            layer_type='crop',
            input_layer=['LayerInput1', 'LayerInput2'],
            output_layer='LayerCrop'
        )

    @unittest.skip(" Radar: 32877551")
    def test_concat_layer(self):
        self.run_case(
            layer_type='concat',
            input_layer=['LayerInput1', 'LayerInput2'],
            output_layer='LayerConcat'
        )

    @pytest.mark.slow
    def test_pooling_layer(self):
        self.run_case(
            layer_type='pooling',
            input_layer='data',
            output_layer='LayerPooling'
        )

    @pytest.mark.slow
    def test_lrn(self):
        self.run_case(
            layer_type='lrn',
            input_layer='data',
            output_layer='LayerLRN',
        )

    @unittest.skip(" Radar: 33056676")
    def test_mvn(self):
        self.run_case(
            layer_type='mvn',
            input_layer='data',
            output_layer='LayerMVN',
        )

    def test_reshape(self):
        self.run_case(
            layer_type='reshape',
            input_layer='data',
            output_layer='LayerReshape',
        )

    def test_embed(self):
        self.run_case(
            layer_type='embed',
            input_layer='data',
            output_layer='LayerEmbed',
        )

    def test_batchnorm(self):
        self.run_case(
            layer_type='batchnorm',
            input_layer='data',
            output_layer='LayerBatchNorm',
        )

    def test_flatten(self):
        self.run_case(
            layer_type='flatten',
            input_layer='data',
            output_layer='LayerFlatten',
        )

    def test_eltwise(self):
        self.run_case(
            layer_type='eltwise',
            input_layer=['LayerInput1', 'LayerInput2'],
            output_layer='LayerEltwise',
        )

    @unittest.skip("Radar: 32739970")
    def test_parameter(self):
        self.run_case(
            layer_type='parameter',
            input_layer='data',
            output_layer='LayerParameter',
        )

    def test_split(self):
        self.run_case(
            layer_type='split',
            input_layer='data',
            output_layer=['LayerOutput_1', 'LayerOutput_2'],
        )
