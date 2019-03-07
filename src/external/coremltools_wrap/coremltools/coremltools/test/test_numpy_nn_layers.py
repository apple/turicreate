from __future__ import print_function
import unittest
import numpy as np
import os
import shutil
import tempfile
import coremltools.models.datatypes as datatypes
from coremltools.models import neural_network as neural_network
from coremltools.models.utils import macos_version
import coremltools
import itertools

np.random.seed(10)

class CorrectnessTest(unittest.TestCase):

    def _compare_shapes(self, np_preds, coreml_preds):
        if np.squeeze(np_preds).shape != np.squeeze(coreml_preds).shape:
            return False
        else:
            return True

    def _compare_predictions(self, np_preds, coreml_preds, delta = .01):
        np_preds = np_preds.flatten()
        coreml_preds = coreml_preds.flatten()
        for i in range(len(np_preds)):
            max_den = max(1.0, np_preds[i], coreml_preds[i])
            if np.abs(np_preds[i] / max_den - coreml_preds[i] / max_den) > delta:
                return False
        return True


def get_size_after_stride(X, params):
    start = params["start"]
    end = params["end"]
    stride = params["stride"]
    if params["axis"] == 'width': axis = 2
    if params["axis"] == 'height': axis = 1
    if params["axis"] == 'channel': axis = 0
    N = X.shape[axis]
    if end < 0: end = end + N
    end = min(end, N)
    if start > N-1:
        L = 0
    else:
        L = np.floor((end - 1 - start)/stride) + 1
        if L<0 : L = 0
    return L

def get_numpy_predictions_slice(X, params):
    start = params["start"]
    end = params["end"]
    stride = params["stride"]
    if params["axis"] == 'width': return X[:,:,start:end:stride]
    if params["axis"] == 'height': return X[:,start:end:stride,:]
    if params["axis"] == 'channel': return X[start:end:stride,:,:]

def get_coreml_predictions_slice(X, params):
    coreml_preds = []
    eval = True
    try:
        input_dim = X.shape
        output_dim = (1, 1, 1) #some random dimensions here: we are going to remove this information later
        input_features = [('data', datatypes.Array(*input_dim))]
        output_features = [('output', datatypes.Array(*output_dim))]
        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        builder.add_slice('slice', 'data', 'output', start_index = params["start"],
                            end_index = params["end"], stride = params["stride"], axis = params["axis"])
        #Remove output shape by deleting and adding an output
        del builder.spec.description.output[-1]
        output = builder.spec.description.output.add()
        output.name = 'output'
        output.type.multiArrayType.dataType = coremltools.proto.FeatureTypes_pb2.ArrayFeatureType.ArrayDataType.Value('DOUBLE')
        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)
        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        coreml_input = {'data': X}
        if macos_version() >= (10, 13):
            coreml_preds = coreml_model.predict(coreml_input)['output']
        else:
            coreml_preds = None
        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)
    except RuntimeError as e:
        print(e)
        eval = False

    return coreml_preds, eval

def get_numpy_predictions_reduce(X, params):
    if params["axis"] == 'CHW': axis = (0,1,2)
    if params["axis"] == 'HW' : axis = (1,2)
    if params["axis"] == 'C'  : axis = 0
    if params["axis"] == 'H'  : axis = 1
    if params["axis"] == 'W'  : axis = 2

    if params["mode"] == 'sum': return np.sum(X, axis)
    if params["mode"] == 'avg': return np.mean(X, axis)
    if params["mode"] == 'prod': return np.prod(X, axis)
    if params["mode"] == 'logsum': return np.sum(np.log(X+1e-6), axis)
    if params["mode"] == 'sumsquare': return np.sum(X ** 2, axis)
    if params["mode"] == 'L2': return np.sqrt(np.sum(X ** 2, axis))
    if params["mode"] == 'L1': return np.sum(np.abs(X), axis)
    if params["mode"] == 'max': return np.amax(X, axis)
    if params["mode"] == 'min': return np.amin(X, axis)
    if params["mode"] == 'argmax': return np.argmax(X, axis)

def get_coreml_predictions_reduce(X, params):
    coreml_preds = []
    eval = True
    try:
        input_dim = X.shape
        output_dim = (1, 1, 1) #some random dimensions here: we are going to remove this information later
        input_features = [('data', datatypes.Array(*input_dim))]
        output_features = [('output', datatypes.Array(*output_dim))]
        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        builder.add_reduce('reduce', 'data', 'output', axis = params["axis"], mode = params["mode"])
        #Remove output shape by deleting and adding an output
        del builder.spec.description.output[-1]
        output = builder.spec.description.output.add()
        output.name = 'output'
        output.type.multiArrayType.dataType = coremltools.proto.FeatureTypes_pb2.ArrayFeatureType.ArrayDataType.Value('DOUBLE')
        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)
        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        coreml_input = {'data': X}
        if macos_version() >= (10, 13):
            coreml_preds = coreml_model.predict(coreml_input)['output']
        else:
            coreml_preds = None
        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)
    except RuntimeError as e:
        print(e)
        eval = False

    return coreml_preds, eval

def get_coreml_predictions_unary(x, mode, alpha = 1.0):

    #create a tiny mlmodel
    input_dim = x.shape
    input_features = [('data', datatypes.Array(*input_dim))]
    output_features = [('output', datatypes.Array(*input_dim))]

    builder = neural_network.NeuralNetworkBuilder(input_features,
            output_features)

    builder.add_unary(name= 'unary', input_name = 'data', output_name = 'output', mode = mode, alpha = alpha)

    #save the model
    model_dir = tempfile.mkdtemp()
    model_path = os.path.join(model_dir, 'test_layer.mlmodel')
    coremltools.utils.save_spec(builder.spec, model_path)

    #preprare input and get predictions
    coreml_model = coremltools.models.MLModel(model_path)
    if macos_version() >= (10, 13):
        coreml_input = {'data': x}
        coreml_preds = coreml_model.predict(coreml_input)['output']
    else:
        coreml_preds = None

    if os.path.exists(model_dir):
        shutil.rmtree(model_dir)

    return coreml_preds


class SimpleTest(CorrectnessTest):

    def test_tiny_upsample_linear_mode(self):

        #create a tiny mlmodel
        input_dim = (1,1,3) #(C,H,W)
        input_features = [('data', datatypes.Array(*input_dim))]
        output_features = [('output', None)]

        builder = neural_network.NeuralNetworkBuilder(input_features,
                output_features)
        builder.add_upsample(name= 'upsample',
                             scaling_factor_h = 2, scaling_factor_w = 3,
                             input_name= 'data', output_name= 'output',
                             mode = 'BILINEAR')

        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)

        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        if macos_version() >= (10, 13):
            coreml_input = {'data': np.reshape(np.array([1.0,2.0,3.0]), (1,1,3))}
            coreml_preds = coreml_model.predict(coreml_input)['output']

            #harcoded for this simple test case
            numpy_preds = np.array([[1, 1.333, 1.666, 2, 2.333, 2.666, 3, 3, 3],\
                    [1, 1.333, 1.6666, 2, 2.33333, 2.6666, 3, 3, 3]])
            #numpy_preds = np.array([[1, 1, 1, 2, 2, 2, 3, 3, 3],[1, 1, 1, 2, 2, 2, 3, 3, 3]])
            #Test
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)

    def test_LRN(self):

        #create a tiny mlmodel
        input_dim = (1,3,3)
        input_features = [('data', datatypes.Array(*input_dim))]
        output_features = [('output', datatypes.Array(*input_dim))]

        builder = neural_network.NeuralNetworkBuilder(input_features,
                output_features)

        builder.add_lrn(name= 'lrn', input_name = 'data', output_name = 'output',
                        alpha = 2, beta = 3, local_size = 1, k = 8)

        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)

        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        if macos_version() >= (10, 13):
            coreml_input = {'data': np.ones((1,3,3))}
            coreml_preds = coreml_model.predict(coreml_input)['output']

            #harcoded for this simple test case
            numpy_preds = 1e-3 * np.ones((1,3,3))
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)

    def test_MVN(self):

        #create a tiny mlmodel
        input_dim = (2,2,2)
        input_features = [('data', datatypes.Array(*input_dim))]
        output_features = [('output', datatypes.Array(*input_dim))]

        builder = neural_network.NeuralNetworkBuilder(input_features,
                output_features)

        builder.add_mvn(name= 'mvn', input_name = 'data', output_name = 'output',
                        across_channels = False, normalize_variance = False)

        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)

        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        if macos_version() >= (10, 13):
            coreml_input = {'data': np.reshape(np.arange(8, dtype=np.float32), (2,2,2))}
            coreml_preds = coreml_model.predict(coreml_input)['output']

            #harcoded for this simple test case
            numpy_preds = np.reshape(np.arange(8) - np.array([1.5,1.5,1.5,1.5,5.5,5.5,5.5,5.5]),(2,2,2))
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)

    def test_L2_normalize(self):

        #create a tiny mlmodel
        input_dim = (1,2,2)
        input_features = [('data', datatypes.Array(*input_dim))]
        output_features = [('output', datatypes.Array(*input_dim))]

        builder = neural_network.NeuralNetworkBuilder(input_features,
                output_features)

        builder.add_l2_normalize(name= 'mvn', input_name = 'data', output_name = 'output')

        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)

        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        if macos_version() >= (10, 13):
            coreml_input = {'data': np.reshape(np.arange(4, dtype=np.float32), (1,2,2))}
            coreml_preds = coreml_model.predict(coreml_input)['output']

            #harcoded for this simple test case
            numpy_preds = np.reshape(np.arange(4, dtype=np.float32), (1,2,2))/np.sqrt(14)
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)

    def test_unary(self):

        x = np.reshape(np.arange(1,5, dtype=np.float32), (1,2,2))

        coreml_preds = get_coreml_predictions_unary(x, 'sqrt')
        if coreml_preds is not None:
            numpy_preds = np.sqrt(x)
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        coreml_preds = get_coreml_predictions_unary(x, 'rsqrt')
        if coreml_preds is not None:
            numpy_preds = 1/np.sqrt(x)
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        coreml_preds = get_coreml_predictions_unary(x, 'inverse')
        if coreml_preds is not None:
            numpy_preds = 1/x
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        coreml_preds = get_coreml_predictions_unary(x, 'power', 3)
        if coreml_preds is not None:
            numpy_preds = x ** 3
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        coreml_preds = get_coreml_predictions_unary(x, 'exp')
        if coreml_preds is not None:
            numpy_preds = np.exp(x)
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        coreml_preds = get_coreml_predictions_unary(x, 'log')
        if coreml_preds is not None:
            numpy_preds = np.log(x)
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        coreml_preds = get_coreml_predictions_unary(x, 'abs')
        if coreml_preds is not None:
            numpy_preds = np.abs(x)
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        coreml_preds = get_coreml_predictions_unary(x, 'threshold', alpha = 2)
        if coreml_preds is not None:
            numpy_preds = np.maximum(x, 2)
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

    def test_split(self):

        #create a tiny mlmodel
        input_dim = (9,2,2)
        x = np.random.rand(*input_dim)

        input_features = [('data', datatypes.Array(*input_dim))]
        output_names = []
        output_features = []
        for i in range(3):
            out = 'out_' + str(i)
            output_names.append(out)
            output_features.append((out, None))

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

        builder.add_split(name= 'split', input_name = 'data', output_names = output_names)

        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)

        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        if macos_version() >= (10, 13):
            coreml_input = {'data': x}
            coreml_preds_dict = coreml_model.predict(coreml_input)

            for i in range(3):
                coreml_preds = coreml_preds_dict[output_names[i]]
                numpy_preds = x[i*3:i*3+3,:,:]
                self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
                self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)

    def test_scale_constant(self):

        #create a tiny mlmodel
        input_dim = (1,2,2)
        input_features = [('data', datatypes.Array(*input_dim))]
        output_features = [('output', None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

        builder.add_scale(name = 'scale', W = 5, b = 45, has_bias = True, input_name = 'data', output_name = 'output')

        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)

        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        if macos_version() >= (10, 13):
            x = np.reshape(np.arange(4, dtype=np.float32), (1,2,2))
            coreml_input = {'data': x}
            coreml_preds = coreml_model.predict(coreml_input)['output']

            #harcoded for this simple test case
            numpy_preds = 5 * x + 45
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)

    def test_scale_matrix(self):

        #create a tiny mlmodel
        input_dim = (1,2,2)
        input_features = [('data', datatypes.Array(*input_dim))]
        output_features = [('output', None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

        W = np.reshape(np.arange(5,9), (1,2,2))

        builder.add_scale(name = 'scale', W = W, b = None, has_bias = False, input_name = 'data', output_name = 'output',
                            shape_scale = [1,2,2])

        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)

        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        if macos_version() >= (10, 13):
            x = np.reshape(np.arange(4, dtype=np.float32), (1,2,2))
            coreml_input = {'data': x}
            coreml_preds = coreml_model.predict(coreml_input)['output']

            #harcoded for this simple test case
            numpy_preds = W * x
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)

    def test_bias_constant(self):

        #create a tiny mlmodel
        input_dim = (1,2,2)
        input_features = [('data', datatypes.Array(*input_dim))]
        output_features = [('output', None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

        builder.add_bias(name = 'bias', b = 45, input_name = 'data', output_name = 'output')

        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)

        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        if macos_version() >= (10, 13):
            x = np.reshape(np.arange(4, dtype=np.float32), (1,2,2))
            coreml_input = {'data': x}
            coreml_preds = coreml_model.predict(coreml_input)['output']

            #harcoded for this simple test case
            numpy_preds = x + 45
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)

    def test_bias_matrix(self):

        #create a tiny mlmodel
        input_dim = (1,2,2)
        input_features = [('data', datatypes.Array(*input_dim))]
        output_features = [('output', None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

        b = np.reshape(np.arange(5,9), (1,2,2))

        builder.add_bias(name = 'bias', b = b, input_name = 'data', output_name = 'output',
                            shape_bias = [1,2,2])

        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)

        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        if macos_version() >= (10, 13):
            x = np.reshape(np.arange(4, dtype=np.float32), (1,2,2))
            coreml_input = {'data': x}
            coreml_preds = coreml_model.predict(coreml_input)['output']

            #harcoded for this simple test case
            numpy_preds = x + b
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)

    def test_load_constant(self):

        #create a tiny mlmodel
        input_dim = (1,2,2)
        input_features = [('data', datatypes.Array(*input_dim))]
        output_features = [('output', None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

        b = np.reshape(np.arange(5,9), (1,2,2))

        builder.add_load_constant(name= 'load_constant', output_name = 'bias', constant_value = b, shape = [1,2,2])
        builder.add_elementwise(name= 'add', input_names = ['data', 'bias'], output_name = 'output', mode = 'ADD')

        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)

        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        if macos_version() >= (10, 13):
            x = np.reshape(np.arange(4, dtype=np.float32), (1,2,2))
            coreml_input = {'data': x}
            coreml_preds = coreml_model.predict(coreml_input)['output']

            #harcoded for this simple test case
            numpy_preds = x + b
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

            # Test half precision case
            coreml_fp16_model = coremltools.utils.convert_neural_network_weights_to_fp16(coreml_model)
            coreml_preds = coreml_fp16_model.predict(coreml_input)['output']
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)

    def test_min(self):

        #create a tiny mlmodel
        input_dim = (1,2,2)
        input_features = [('data_0', datatypes.Array(*input_dim)), ('data_1', datatypes.Array(*input_dim))]
        output_features = [('output', None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

        builder.add_elementwise(name = 'min', input_names= ['data_0', 'data_1'], output_name = 'output', mode = 'MIN')

        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)

        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        if macos_version() >= (10, 13):
            x1 = np.reshape(np.arange(4, dtype=np.float32), (1,2,2))
            x2 = np.reshape(np.arange(2,6, dtype=np.float32), (1,2,2))
            coreml_input = {'data_0': x1, 'data_1': x2}
            coreml_preds = coreml_model.predict(coreml_input)['output']

            #harcoded for this simple test case
            numpy_preds = np.minimum(x1,x2)
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)

    def test_conv_same_padding(self):

        #create a tiny mlmodel
        input_dim = (10,15,15)
        input_features = [('data', datatypes.Array(*input_dim))]
        output_features = [('output', None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

        W = np.random.rand(3,3,10,20)

        builder.add_convolution(name = 'conv', kernel_channels = 10, output_channels = 20,
                        height = 3, width = 3, stride_height = 2, stride_width = 2,
                        border_mode = 'same', groups = 1,
                        W = W, b = None, has_bias = False,
                        input_name = 'data', output_name = 'output',
                        same_padding_asymmetry_mode = 'TOP_LEFT_HEAVY')

        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)

        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        if macos_version() >= (10, 13):
            x = np.random.rand(*input_dim)
            coreml_input = {'data': x}
            coreml_preds = coreml_model.predict(coreml_input)['output']

            #harcoded for this simple test case
            numpy_preds = np.random.rand(20,8,8)
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)

    def test_deconv_valid_padding(self):

        #create a tiny mlmodel
        input_dim = (10,15,15)
        input_features = [('data', datatypes.Array(*input_dim))]
        output_features = [('output', None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

        W = np.random.rand(3,3,20,10)

        builder.add_convolution(name = 'deconv', kernel_channels = 10, output_channels = 20,
                        height = 3, width = 3, stride_height = 2, stride_width = 2,
                        border_mode = 'valid', groups = 1,
                        W = W, b = None, has_bias = False,
                        is_deconv = True,
                        input_name = 'data', output_name = 'output',
                        padding_top = 2, padding_bottom = 3, padding_left = 2, padding_right = 3)

        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)

        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        if macos_version() >= (10, 13):
            x = np.random.rand(*input_dim)
            coreml_input = {'data': x}
            coreml_preds = coreml_model.predict(coreml_input)['output']

            #harcoded for this simple test case
            numpy_preds = np.random.rand(20,26,26)
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)


    def test_linear_activation(self):

        #create a tiny mlmodel
        input_dim = (10,15,15)
        input_features = [('data', datatypes.Array(*input_dim))]
        output_features = [('output', None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

        builder.add_activation(name = 'activation',
                               non_linearity = 'LINEAR',
                               input_name = 'data',
                               output_name = 'output', params= [34.0, 67.0])

        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)

        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        if macos_version() >= (10, 13):
            x = np.random.rand(*input_dim)
            coreml_input = {'data': x}
            coreml_preds = coreml_model.predict(coreml_input)['output']

            #harcoded for this simple test case
            numpy_preds = 34.0 * x + 67.0
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)


    def test_padding_constant(self):

        #create a tiny mlmodel
        input_dim = (1,2,3)
        input_features = [('data', datatypes.Array(*input_dim))]
        output_features = [('output', None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

        builder.add_padding(name = 'pad',
                            left = 1, right = 0, top = 2, bottom = 0,
                            value = -1,
                            input_name = 'data',
                            output_name = 'output')

        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)

        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        if macos_version() >= (10, 13):
            x = np.reshape(np.array([[1,2,3], [4,5,6]]), (1,2,3)).astype(np.float32)
            coreml_input = {'data': x}
            coreml_preds = coreml_model.predict(coreml_input)['output']

            #harcoded for this simple test case
            numpy_preds = np.reshape(np.array([[-1,-1,-1,-1], [-1,-1,-1,-1], [-1,1,2,3], [-1,4,5,6]]), (1,4,4)).astype(np.float32)
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)


    def test_padding_replication(self):

        #create a tiny mlmodel
        input_dim = (1,2,3)
        input_features = [('data', datatypes.Array(*input_dim))]
        output_features = [('output', None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

        builder.add_padding(name = 'pad',
                            left = 1, top = 2,
                            input_name = 'data',
                            output_name = 'output', padding_type = 'replication')

        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)

        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        if macos_version() >= (10, 13):
            x = np.reshape(np.array([[1,2,3], [4,5,6]]), (1,2,3)).astype(np.float32)
            coreml_input = {'data': x}
            coreml_preds = coreml_model.predict(coreml_input)['output']

            #harcoded for this simple test case
            numpy_preds = np.reshape(np.array([[1,1,2,3], [1,1,2,3], [1,1,2,3], [4,4,5,6]]), (1,4,4)).astype(np.float32)
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)


    def test_reshape_target_shape_3(self):

        #create a tiny mlmodel
        input_dim = (1,2,5) #(C,H,W)
        input_features = [('data', datatypes.Array(*input_dim))]
        output_features = [('output', None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

        builder.add_reshape(name = 'reshape', input_name = 'data', output_name = 'output', target_shape = (10,1,1), mode = 0)

        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)

        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        if macos_version() >= (10, 13):
            x = np.random.rand(*input_dim)
            coreml_input = {'data': x}
            coreml_preds = coreml_model.predict(coreml_input)['output']

            #harcoded for this simple test case
            numpy_preds = np.reshape(x, (10,1,1))
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)

    def test_reshape_target_shape_4(self):

        #create a tiny mlmodel
        input_dim = (1,2,5) #(C,H,W)
        input_features = [('data', datatypes.Array(*input_dim))]
        output_features = [('output', None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

        builder.add_reshape(name = 'reshape', input_name = 'data', output_name = 'output', target_shape = (1,10,1,1), mode = 0)

        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)

        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        if macos_version() >= (10, 13):
            x = np.random.rand(*input_dim)
            coreml_input = {'data': x}
            coreml_preds = coreml_model.predict(coreml_input)['output']

            #harcoded for this simple test case
            numpy_preds = np.reshape(x, (1,10,1,1))
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)


class SimpleTestCPUOnly(CorrectnessTest):

    def test_bias_matrix_CPU(self):

        #create a tiny mlmodel
        input_dim = (1,2,2)
        input_features = [('data', datatypes.Array(*input_dim))]
        output_features = [('output', None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

        b = np.reshape(np.arange(5,9), (1,2,2))

        builder.add_bias(name = 'bias', b = b, input_name = 'data', output_name = 'output',
                            shape_bias = [1,2,2])

        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)

        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        if macos_version() >= (10, 13):
            x = np.reshape(np.arange(4, dtype=np.float32), (1,2,2))
            coreml_input = {'data': x}
            coreml_preds = coreml_model.predict(coreml_input, useCPUOnly = True)['output']

            #harcoded for this simple test case
            numpy_preds = x + b
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)


    def test_linear_activation_CPU(self):

        #create a tiny mlmodel
        input_dim = (10,15,15)
        input_features = [('data', datatypes.Array(*input_dim))]
        output_features = [('output', None)]

        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

        builder.add_activation(name = 'activation',
                               non_linearity = 'LINEAR',
                               input_name = 'data',
                               output_name = 'output', params= [34.0, 67.0])

        #save the model
        model_dir = tempfile.mkdtemp()
        model_path = os.path.join(model_dir, 'test_layer.mlmodel')
        coremltools.utils.save_spec(builder.spec, model_path)

        #preprare input and get predictions
        coreml_model = coremltools.models.MLModel(model_path)
        if macos_version() >= (10, 13):
            x = np.random.rand(*input_dim)
            coreml_input = {'data': x}
            coreml_preds = coreml_model.predict(coreml_input, useCPUOnly = True)['output']

            #harcoded for this simple test case
            numpy_preds = 34.0 * x + 67.0
            self.assertTrue(self._compare_shapes(numpy_preds, coreml_preds))
            self.assertTrue(self._compare_predictions(numpy_preds, coreml_preds))

        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)



class StressTest(CorrectnessTest):

    def test_slice_layer(self):
        '''
        Define Params
        '''
        params_dict = dict(
                         input_shape = [[30,100,8], [80,50,5], [4,12,5], [56,8,14]],
                         axis = ['channel', 'height', 'width'],
                         start = [0,1,2,5],
                         end = [5,100,56,-1,-2,-4],
                         stride = [1,2,3]
                         )
        params = list(itertools.product(*params_dict.values()))
        all_candidates = [dict(zip(params_dict.keys(), x)) for x in params]
        valid_params = []
        for pr in all_candidates:
            X = np.random.rand(*pr["input_shape"])
            if get_size_after_stride(X, pr):
                valid_params.append(pr)
        print("Total params to be tested: ", len(valid_params), "out of canditates: ", len(all_candidates))
        '''
        Test
        '''
        failed_tests_compile = []
        failed_tests_shape = []
        failed_tests_numerical = []
        for i in range(len(valid_params)):
            params = valid_params[i]
            #print("=========: ", params)
            #if i % 10 == 0: print("======== Testing {}/{}".format(str(i), str(len(valid_params))))
            X = np.random.rand(*params["input_shape"])
            np_preds = get_numpy_predictions_slice(X, params)
            coreml_preds, eval = get_coreml_predictions_slice(X, params)
            if eval is False:
                failed_tests_compile.append(params)
            elif coreml_preds is not None:
                if not self._compare_shapes(np_preds, coreml_preds):
                    failed_tests_shape.append(params)
                elif not self._compare_predictions(np_preds, coreml_preds):
                    failed_tests_numerical.append(params)

        self.assertEqual(failed_tests_compile,[])
        self.assertEqual(failed_tests_shape, [])
        self.assertEqual(failed_tests_numerical,[])

    def test_reduce_layer(self):
        '''
        Define Params
        '''
        if 1:
            params_dict = dict(
                       input_shape = [[3,10,8], [8,5,5], [4,12,10], [7,1,14]],
                       mode = ['sum', 'avg', 'prod', 'sumsquare', 'L1', 'L2', 'max', 'min', 'argmax'],
                       axis = ['CHW', 'HW', 'C', 'H', 'W'],
                       )
        if 0:
            params_dict = dict(
                       input_shape = [[3,10,8]],
                       mode = ['logsum'],
                       axis = ['HW'],
                       )
        params = list(itertools.product(*params_dict.values()))
        all_candidates = [dict(zip(params_dict.keys(), x)) for x in params]
        valid_params = []
        for pr in all_candidates:
            if pr["mode"] == 'argmax':
                if pr["axis"] == 'CHW' or pr["axis"] == 'HW':
                    continue
            valid_params.append(pr)
        print("Total params to be tested: ", len(valid_params), "out of canditates: ", len(all_candidates))
        '''
        Test
        '''
        failed_tests_compile = []
        failed_tests_shape = []
        failed_tests_numerical = []
        for i in range(len(valid_params)):
            params = valid_params[i]
            #print("=========: ", params)
            #if i % 10 == 0: print("======== Testing {}/{}".format(str(i), str(len(valid_params))))
            X = np.random.rand(*params["input_shape"])
            np_preds = get_numpy_predictions_reduce(X, params)
            coreml_preds, eval = get_coreml_predictions_reduce(X, params)
            if eval is False:
                failed_tests_compile.append(params)
            elif coreml_preds is not None:
                if not self._compare_shapes(np_preds, coreml_preds):
                    failed_tests_shape.append(params)
                elif not self._compare_predictions(np_preds, coreml_preds):
                    failed_tests_numerical.append(params)

        self.assertEqual(failed_tests_compile,[])
        self.assertEqual(failed_tests_shape, [])
        self.assertEqual(failed_tests_numerical,[])
