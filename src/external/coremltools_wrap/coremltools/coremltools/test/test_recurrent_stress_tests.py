import unittest
import numpy as np
import itertools
from coremltools._deps import HAS_KERAS2_TF, HAS_KERAS_TF
from coremltools.models.utils import macos_version
import pytest
from copy import copy

np.random.seed(1377)

if HAS_KERAS2_TF or HAS_KERAS_TF:
    import keras
    from keras.models import Sequential, Model
    from keras.layers import LSTM, GRU, SimpleRNN, RepeatVector
    from keras.layers.wrappers import Bidirectional, TimeDistributed
    import keras.backend as K
    from coremltools.converters import keras as keras_converter

'''
=============================
Utility Functions
=============================
'''


def get_recurrent_activation_name_from_keras(activation):
    if activation == keras.activations.sigmoid:
        activation_str = 'SIGMOID'
    elif activation == keras.activations.hard_sigmoid:
        activation_str = 'SIGMOID_HARD'
    elif activation == keras.activations.tanh:
        activation_str = 'TANH'
    elif activation == keras.activations.relu:
        activation_str = 'RELU'
    elif activation == keras.activations.linear:
        activation_str = 'LINEAR'
    else:
        raise NotImplementedError('activation %s not supported for Recurrent layer.' % activation)
    return activation_str


def linear(x, alpha=1, beta=0):
    return alpha * x + beta


def relu(x):
    return np.maximum(0, x)


def sigmoid(x):
    return 1. / (1 + np.exp(-x))


def hard_sigmoid(x, alpha=0.2, beta=0.5):
    return np.minimum(np.maximum(alpha * x + beta, 0), 1)


def tanh(x):
    return np.tanh(x)


def apply_act(x, option):
    if option == 'TANH':
        return tanh(x)
    elif option == 'RELU':
        return relu(x)
    elif option == 'SIGMOID':
        return sigmoid(x)
    elif option == 'SIGMOID_HARD':
        return hard_sigmoid(x)
    elif option == 'LINEAR':
        return linear(x)


def clip(x, threshold=50.0):
    return np.maximum(np.minimum(x, threshold), -threshold)


def valid_params(params):
    """Checks if this combination of parameters is allowed by Keras"""
    return not (
        params['input_dims'][1] == 1 and
        params['unroll']
    )

'''
=============================
Numpy implementations
=============================
'''


def get_numpy_prediction_gru(model, X):
    X = X[0, :, :]
    seq_len, input_size = X.shape
    keras_layer = model.layers[0]
    return_seq = keras_layer.return_sequences
    if keras_layer.go_backwards:
        X = X[::-1, :]

    if HAS_KERAS2_TF:
        hidden_size = keras_layer.units

        keras_W_h = keras_layer.get_weights()[1].T
        R_z = keras_W_h[0 * hidden_size:][:hidden_size]
        R_r = keras_W_h[1 * hidden_size:][:hidden_size]
        R_o = keras_W_h[2 * hidden_size:][:hidden_size]

        keras_W_x = keras_layer.get_weights()[0].T
        W_z = keras_W_x[0 * hidden_size:][:hidden_size]
        W_r = keras_W_x[1 * hidden_size:][:hidden_size]
        W_o = keras_W_x[2 * hidden_size:][:hidden_size]

        keras_b = keras_layer.get_weights()[2]
        b_z = keras_b[0 * hidden_size:][:hidden_size]
        b_r = keras_b[1 * hidden_size:][:hidden_size]
        b_o = keras_b[2 * hidden_size:][:hidden_size]

        inner_activation_str = get_recurrent_activation_name_from_keras(keras_layer.recurrent_activation)
        activation_str = get_recurrent_activation_name_from_keras(keras_layer.activation)

    else:
        hidden_size = keras_layer.output_dim

        W_z = keras_layer.get_weights()[0].T
        W_r = keras_layer.get_weights()[3].T
        W_o = keras_layer.get_weights()[6].T

        R_z = keras_layer.get_weights()[1].T
        R_r = keras_layer.get_weights()[4].T
        R_o = keras_layer.get_weights()[7].T

        b_z = keras_layer.get_weights()[2]
        b_r = keras_layer.get_weights()[5]
        b_o = keras_layer.get_weights()[8]

        inner_activation_str = get_recurrent_activation_name_from_keras(keras_layer.inner_activation)
        activation_str = get_recurrent_activation_name_from_keras(keras_layer.activation)

    h = np.zeros((hidden_size))
    c = np.zeros((hidden_size))
    np_out = np.zeros((seq_len, hidden_size))
    for k in range(seq_len):
        x = X[k, :]
        z = apply_act(clip(np.dot(W_z, x) + np.dot(R_z, h) + b_z), inner_activation_str)
        r = apply_act(clip(np.dot(W_r, x) + np.dot(R_r, h) + b_r), inner_activation_str)
        c = clip(h * r)
        o = apply_act(clip(np.dot(W_o, x) + np.dot(R_o, c) + b_o), activation_str)
        h = (1 - z) * o + z * h
        np_out[k, :] = h

    if return_seq:
        np_out_final = np_out
    else:
        np_out_final = np_out[-1, :]
    return np_out_final


def get_numpy_prediction_unilstm(model, X):
    X = X[0, :, :]
    seq_len, input_size = X.shape
    keras_layer = model.layers[0]
    return_seq = keras_layer.return_sequences
    if keras_layer.go_backwards:
        X = X[::-1, :]

    if HAS_KERAS2_TF:
        hidden_size = keras_layer.units

        keras_W_h = keras_layer.get_weights()[1].T
        R_i = keras_W_h[0 * hidden_size:][:hidden_size]
        R_f = keras_W_h[1 * hidden_size:][:hidden_size]
        R_o = keras_W_h[3 * hidden_size:][:hidden_size]
        R_g = keras_W_h[2 * hidden_size:][:hidden_size]

        keras_W_x = keras_layer.get_weights()[0].T
        W_i = keras_W_x[0 * hidden_size:][:hidden_size]
        W_f = keras_W_x[1 * hidden_size:][:hidden_size]
        W_o = keras_W_x[3 * hidden_size:][:hidden_size]
        W_g = keras_W_x[2 * hidden_size:][:hidden_size]

        keras_b = keras_layer.get_weights()[2]
        b_i = keras_b[0 * hidden_size:][:hidden_size]
        b_f = keras_b[1 * hidden_size:][:hidden_size]
        b_o = keras_b[3 * hidden_size:][:hidden_size]
        b_g = keras_b[2 * hidden_size:][:hidden_size]

        inner_activation_str = get_recurrent_activation_name_from_keras(keras_layer.recurrent_activation)
        activation_str = get_recurrent_activation_name_from_keras(keras_layer.activation)

    else:
        hidden_size = keras_layer.output_dim

        R_i = keras_layer.get_weights()[1].T
        R_f = keras_layer.get_weights()[7].T
        R_o = keras_layer.get_weights()[10].T
        R_g = keras_layer.get_weights()[4].T

        W_i = keras_layer.get_weights()[0].T
        W_f = keras_layer.get_weights()[6].T
        W_o = keras_layer.get_weights()[9].T
        W_g = keras_layer.get_weights()[3].T

        b_i = keras_layer.get_weights()[2]
        b_f = keras_layer.get_weights()[8]
        b_o = keras_layer.get_weights()[11]
        b_g = keras_layer.get_weights()[5]

        inner_activation_str = get_recurrent_activation_name_from_keras(keras_layer.inner_activation)
        activation_str = get_recurrent_activation_name_from_keras(keras_layer.activation)

    h = np.zeros((hidden_size))
    c = np.zeros((hidden_size))
    np_out = np.zeros((seq_len, hidden_size))
    for k in range(seq_len):
        x = X[k, :]
        i = apply_act(clip(np.dot(W_i, x) + np.dot(R_i, h) + b_i), inner_activation_str)
        f = apply_act(clip(np.dot(W_f, x) + np.dot(R_f, h) + b_f), inner_activation_str)
        g = apply_act(clip(np.dot(W_g, x) + np.dot(R_g, h) + b_g), activation_str)
        c = c * f + i * g
        c = clip(c, 50000.0)
        o = apply_act(clip(np.dot(W_o, x) + np.dot(R_o, h) + b_o), inner_activation_str)
        h = o * apply_act(c, activation_str)
        np_out[k, :] = h
    if return_seq:
        np_out_final = np_out
    else:
        np_out_final = np_out[-1, :]
    return np_out_final


def get_numpy_prediction_bilstm(model, X):
    X = X[0, :, :]
    seq_len, input_size = X.shape
    keras_layer = model.layers[0]
    return_seq = keras_layer.return_sequences

    if HAS_KERAS2_TF:
        hidden_size = keras_layer.forward_layer.units

        keras_W_h = keras_layer.forward_layer.get_weights()[1].T
        R_i = keras_W_h[0 * hidden_size:][:hidden_size]
        R_f = keras_W_h[1 * hidden_size:][:hidden_size]
        R_o = keras_W_h[3 * hidden_size:][:hidden_size]
        R_g = keras_W_h[2 * hidden_size:][:hidden_size]

        keras_W_x = keras_layer.forward_layer.get_weights()[0].T
        W_i = keras_W_x[0 * hidden_size:][:hidden_size]
        W_f = keras_W_x[1 * hidden_size:][:hidden_size]
        W_o = keras_W_x[3 * hidden_size:][:hidden_size]
        W_g = keras_W_x[2 * hidden_size:][:hidden_size]

        keras_b = keras_layer.forward_layer.get_weights()[2]
        b_i = keras_b[0 * hidden_size:][:hidden_size]
        b_f = keras_b[1 * hidden_size:][:hidden_size]
        b_o = keras_b[3 * hidden_size:][:hidden_size]
        b_g = keras_b[2 * hidden_size:][:hidden_size]

        keras_W_h = keras_layer.backward_layer.get_weights()[1].T
        R_i_back = keras_W_h[0 * hidden_size:][:hidden_size]
        R_f_back = keras_W_h[1 * hidden_size:][:hidden_size]
        R_o_back = keras_W_h[3 * hidden_size:][:hidden_size]
        R_g_back = keras_W_h[2 * hidden_size:][:hidden_size]

        keras_W_x = keras_layer.backward_layer.get_weights()[0].T
        W_i_back = keras_W_x[0 * hidden_size:][:hidden_size]
        W_f_back = keras_W_x[1 * hidden_size:][:hidden_size]
        W_o_back = keras_W_x[3 * hidden_size:][:hidden_size]
        W_g_back = keras_W_x[2 * hidden_size:][:hidden_size]

        keras_b = keras_layer.backward_layer.get_weights()[2]
        b_i_back = keras_b[0 * hidden_size:][:hidden_size]
        b_f_back = keras_b[1 * hidden_size:][:hidden_size]
        b_o_back = keras_b[3 * hidden_size:][:hidden_size]
        b_g_back = keras_b[2 * hidden_size:][:hidden_size]

        inner_activation_str = get_recurrent_activation_name_from_keras(keras_layer.forward_layer.recurrent_activation)
        activation_str = get_recurrent_activation_name_from_keras(keras_layer.forward_layer.activation)

    else:
        hidden_size = keras_layer.forward_layer.output_dim

        R_i = keras_layer.get_weights()[1].T
        R_f = keras_layer.get_weights()[7].T
        R_o = keras_layer.get_weights()[10].T
        R_g = keras_layer.get_weights()[4].T

        W_i = keras_layer.get_weights()[0].T
        W_f = keras_layer.get_weights()[6].T
        W_o = keras_layer.get_weights()[9].T
        W_g = keras_layer.get_weights()[3].T

        b_i = keras_layer.get_weights()[2]
        b_f = keras_layer.get_weights()[8]
        b_o = keras_layer.get_weights()[11]
        b_g = keras_layer.get_weights()[5]

        R_i_back = keras_layer.backward_layer.get_weights()[1].T
        R_f_back = keras_layer.backward_layer.get_weights()[7].T
        R_o_back = keras_layer.backward_layer.get_weights()[10].T
        R_g_back = keras_layer.backward_layer.get_weights()[4].T

        W_i_back = keras_layer.backward_layer.get_weights()[0].T
        W_f_back = keras_layer.backward_layer.get_weights()[6].T
        W_o_back = keras_layer.backward_layer.get_weights()[9].T
        W_g_back = keras_layer.backward_layer.get_weights()[3].T

        b_i_back = keras_layer.backward_layer.get_weights()[2]
        b_f_back = keras_layer.backward_layer.get_weights()[8]
        b_o_back = keras_layer.backward_layer.get_weights()[11]
        b_g_back = keras_layer.backward_layer.get_weights()[5]

        inner_activation_str = get_recurrent_activation_name_from_keras(keras_layer.forward_layer.inner_activation)
        activation_str = get_recurrent_activation_name_from_keras(keras_layer.forward_layer.activation)

    h = np.zeros((hidden_size))
    c = np.zeros((hidden_size))
    np_out_forward = np.zeros((seq_len, hidden_size))
    for k in range(seq_len):
        x = X[k, :]
        i = apply_act(clip(np.dot(W_i, x) + np.dot(R_i, h) + b_i), inner_activation_str)
        f = apply_act(clip(np.dot(W_f, x) + np.dot(R_f, h) + b_f), inner_activation_str)
        g = apply_act(clip(np.dot(W_g, x) + np.dot(R_g, h) + b_g), activation_str)
        c = c * f + i * g
        c = clip(c, 50000.0)
        o = apply_act(clip(np.dot(W_o, x) + np.dot(R_o, h) + b_o), inner_activation_str)
        h = o * apply_act(c, activation_str)
        np_out_forward[k, :] = h

    h = np.zeros((hidden_size))
    c = np.zeros((hidden_size))
    np_out_backward = np.zeros((seq_len, hidden_size))
    for k in range(seq_len):
        x = X[seq_len - k - 1, :]
        i = apply_act(clip(np.dot(W_i_back, x) + np.dot(R_i_back, h) + b_i_back), inner_activation_str)
        f = apply_act(clip(np.dot(W_f_back, x) + np.dot(R_f_back, h) + b_f_back), inner_activation_str)
        g = apply_act(clip(np.dot(W_g_back, x) + np.dot(R_g_back, h) + b_g_back), activation_str)
        c = c * f + i * g
        c = clip(c, 50000.0)
        o = apply_act(clip(np.dot(W_o_back, x) + np.dot(R_o_back, h) + b_o_back), inner_activation_str)
        h = o * apply_act(c, activation_str)
        np_out_backward[k, :] = h

    if return_seq:
        np_out_final = np.zeros((seq_len, 2 * hidden_size))
        for k in range(seq_len):
            np_out_final[k, :hidden_size] = np_out_forward[k, :]
            np_out_final[k, hidden_size:] = np_out_backward[seq_len - k - 1, :]
    else:
        np_out_final = np.zeros((1, 2 * hidden_size))
        np_out_final[0, :hidden_size] = np_out_forward[-1, :]
        np_out_final[0, hidden_size:] = np_out_backward[-1, :]
    return np_out_final


'''
=============================
Nosetest Functions
=============================
'''


def get_mlkit_model_from_path(model):
    from coremltools.converters import keras as keras_converter
    model = keras_converter.convert(model, ['data'], ['output'])
    return model


def generate_input(dim0, dim1, dim2):
    input_data = np.random.rand(dim0, dim1, dim2).astype('f')  # astype() should be removed after radar://31569743
    return input_data


def simple_model_eval(params, model):
    mlkitmodel = get_mlkit_model_from_path(model)
    # New test case takes in 2D input as opposed to uniform 3d input across all other tests
    if len(params[0]['input_dims']) == 3:
        input_data = generate_input(params[0]['input_dims'][0], params[0]['input_dims'][1],
                                    params[0]['input_dims'][2])
        keras_preds = model.predict(input_data).flatten()
    elif len(params[0]['input_dims']) == 2:
        input_data = np.squeeze(np.random.rand(params[0]['input_dims'][0], params[0]['input_dims'][1]))
        keras_preds = model.predict(
            input_data.reshape((params[0]['input_dims'][0], params[0]['input_dims'][1]))).flatten()
    if len(params[0]['input_dims']) == 3:
        input_data = np.transpose(input_data, [1, 0, 2])
    if macos_version() >= (10, 13):
        coreml_preds = mlkitmodel.predict({'data': input_data})['output'].flatten()
        if K.tensorflow_backend._SESSION:
            import tensorflow as tf
            tf.reset_default_graph()
            K.tensorflow_backend._SESSION.close()
            K.tensorflow_backend._SESSION = None

        max_denominator = np.maximum(np.maximum(np.abs(coreml_preds), np.abs(keras_preds)), 1.0)
        relative_error = coreml_preds / max_denominator - keras_preds / max_denominator
        return relative_error, keras_preds, coreml_preds
    else:
        return [], None, None


class SimpleTestCase(unittest.TestCase):
    """
    Test Simple test cases to make sure layers work under basic params. Also, template for testing
    different failing test cases from stress tests
    """

    def _test_simple_rnn(self, keras_major_version):
        params = dict(
            input_dims=[1, 2, 100], go_backwards=False, activation='tanh',
            stateful=False, unroll=False, return_sequences=True, output_dim=4  # Passes for < 3
        ),
        model = Sequential()
        if keras_major_version == 2:
            model.add(SimpleRNN(units=params[0]['output_dim'],
                                input_shape=(params[0]['input_dims'][1],params[0]['input_dims'][2]),
                                activation=params[0]['activation'],
                                return_sequences=params[0]['return_sequences'],
                                go_backwards=params[0]['go_backwards'],
                                unroll=True,
                                ))
        else:
            model.add(SimpleRNN(output_dim=params[0]['output_dim'],
                                input_length=params[0]['input_dims'][1],
                                input_dim=params[0]['input_dims'][2],
                                activation=params[0]['activation'],
                                return_sequences=params[0]['return_sequences'],
                                go_backwards=params[0]['go_backwards'],
                                unroll=True,
                                ))
        relative_error, keras_preds, coreml_preds = simple_model_eval(params, model)
        for i in range(len(relative_error)):
            self.assertLessEqual(relative_error[i], 0.01)

    def _test_simple_lstm(self, keras_major_version):
        params = dict(
            input_dims=[1, 3, 5], go_backwards=True, activation='linear',
            stateful=False, unroll=False, return_sequences=False, output_dim=3,
            inner_activation='linear'
        ),
        model = Sequential()
        if keras_major_version == 2:
            model.add(LSTM(units=params[0]['output_dim'],
                           input_shape=(params[0]['input_dims'][1],params[0]['input_dims'][2]),
                           activation=params[0]['activation'],
                           return_sequences=params[0]['return_sequences'],
                           go_backwards=params[0]['go_backwards'],
                           unroll=True,
                           recurrent_activation='linear'
                           ))
        else:
            model.add(LSTM(output_dim=params[0]['output_dim'],
                           input_length=params[0]['input_dims'][1],
                           input_dim=params[0]['input_dims'][2],
                           activation=params[0]['activation'],
                           return_sequences=params[0]['return_sequences'],
                           go_backwards=params[0]['go_backwards'],
                           unroll=True,
                           inner_activation='linear'
                           ))
        relative_error, keras_preds, coreml_preds = simple_model_eval(params, model)
        for i in range(len(relative_error)):
            self.assertLessEqual(relative_error[i], 0.01)

    def _test_simple_gru(self, keras_major_version):
        params = dict(
            input_dims=[1, 4, 8], go_backwards=False, activation='tanh',
            stateful=False, unroll=False, return_sequences=False, output_dim=4
        ),
        model = Sequential()
        if keras_major_version == 2:
             model.add(GRU(units=params[0]['output_dim'],
                           input_shape=(params[0]['input_dims'][1],params[0]['input_dims'][2]),
                           activation=params[0]['activation'],
                           recurrent_activation='sigmoid',
                           return_sequences=params[0]['return_sequences'],
                           go_backwards=params[0]['go_backwards'],
                           unroll=True,
                           ))
        else:
            model.add(GRU(output_dim=params[0]['output_dim'],
                          input_length=params[0]['input_dims'][1],
                          input_dim=params[0]['input_dims'][2],
                          activation=params[0]['activation'],
                          inner_activation='sigmoid',
                          return_sequences=params[0]['return_sequences'],
                          go_backwards=params[0]['go_backwards'],
                          unroll=True,
                          ))
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])
        relative_error, keras_preds, coreml_preds = simple_model_eval(params, model)
        for i in range(len(relative_error)):
            self.assertLessEqual(relative_error[i], 0.01)

    @unittest.skipIf(not HAS_KERAS_TF, 'Missing keras 1. Skipping test.')
    @pytest.mark.keras1
    def test_keras1_simple_rnn(self):
        self._test_simple_rnn(keras_major_version=1)

    @unittest.skipIf(not HAS_KERAS_TF, 'Missing keras 1. Skipping test.')
    @pytest.mark.keras1
    def test_keras1_simple_lstm(self):
        self._test_simple_lstm(keras_major_version=1)

    @unittest.skipIf(not HAS_KERAS_TF, 'Missing keras 1. Skipping test.')
    @pytest.mark.keras1
    def test_keras1_simple_gru(self):
        self._test_simple_gru(keras_major_version=1)

    @unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras 2. Skipping test.')
    @pytest.mark.keras2
    def test_keras2_simple_rnn(self):
        self._test_simple_rnn(keras_major_version=2)

    @unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras 2. Skipping test.')
    @pytest.mark.keras2
    def test_keras2_simple_lstm(self):
        self._test_simple_lstm(keras_major_version=2)

    @unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras 2. Skipping test.')
    @pytest.mark.keras2
    def test_keras2_simple_gru(self):
        self._test_simple_gru(keras_major_version=2)


class RecurrentLayerTest(unittest.TestCase):
    """
    Base class for recurrent layer tests. Masking param not included here
    """

    def setUp(self):
        self.params_dict = dict(
            input_dims=[[1, 5, 10], [1, 1, 1], [1, 2, 5]],
            output_dim=[1, 5, 10],
            stateful=[False],
            go_backwards=[False, True],
            unroll=[True],
            return_sequences=[False, True],
            activation=['tanh', 'linear', 'sigmoid', 'hard_sigmoid', 'relu'],
        )
        self.base_layer_params = list(itertools.product(*self.params_dict.values()))


class RNNLayer(RecurrentLayerTest):
    """
    Class for testing single RNN layer
    """

    def setUp(self):
        super(RNNLayer, self).setUp()
        self.simple_rnn_params_dict = self.params_dict
        self.rnn_layer_params = list(itertools.product(self.simple_rnn_params_dict.values()))

    def _test_rnn_layer(self, keras_major_version, limit=None):
        i = 0
        numerical_err_models = []
        shape_err_models = []
        numerical_failiure = 0
        params = list(itertools.product(self.base_layer_params, self.rnn_layer_params))
        np.random.shuffle(params)
        params = [param for param in params if valid_params(dict(zip(self.params_dict.keys(), param[0])))]
        for base_params, rnn_params in params:
            base_params = dict(zip(self.params_dict.keys(), base_params))
            rnn_params = dict(zip(self.simple_rnn_params_dict.keys(), rnn_params))
            model = Sequential()
            unroll = base_params['unroll']
            if base_params['input_dims'][1] == 1 and unroll == True:
                unroll = False
            if keras_major_version == 2:
                model.add(
                    SimpleRNN(
                        base_params['output_dim'],
                        input_shape=base_params['input_dims'][1:],
                        activation=base_params['activation'],
                        return_sequences=base_params['return_sequences'],
                        go_backwards=base_params['go_backwards'],
                        unroll=unroll,
                    )
                )
            else:
                model.add(
                    SimpleRNN(
                        base_params['output_dim'],
                        input_length=base_params['input_dims'][1],
                        input_dim=base_params['input_dims'][2],
                        activation=base_params['activation'],
                        return_sequences=base_params['return_sequences'],
                        go_backwards=base_params['go_backwards'],
                        unroll=unroll,
                    )
                )
            mlkitmodel = get_mlkit_model_from_path(model)
            input_data = generate_input(base_params['input_dims'][0], base_params['input_dims'][1],
                                        base_params['input_dims'][2])
            keras_preds = model.predict(input_data).flatten()
            if K.tensorflow_backend._SESSION:
                import tensorflow as tf
                tf.reset_default_graph()
                K.tensorflow_backend._SESSION.close()
                K.tensorflow_backend._SESSION = None
            input_data = np.transpose(input_data, [1, 0, 2])
            if macos_version() >= (10, 13):
                coreml_preds = mlkitmodel.predict({'data': input_data})['output'].flatten()
                try:
                    self.assertEquals(coreml_preds.shape, keras_preds.shape)
                except AssertionError:
                    print("Shape error:\nbase_params: {}\nkeras_preds.shape: {}\ncoreml_preds.shape: {}".format(
                        base_params, keras_preds.shape, coreml_preds.shape))
                    shape_err_models.append(base_params)
                    i += 1
                    continue
                try:
                    max_denominator = np.maximum(np.maximum(np.abs(coreml_preds), np.abs(keras_preds)), 1.0)
                    relative_error = coreml_preds / max_denominator - keras_preds / max_denominator
                    for i in range(len(relative_error)):
                        self.assertLessEqual(relative_error[i], 0.01)
                except AssertionError:
                    print("Assertion error:\nbase_params: {}\nkeras_preds: {}\ncoreml_preds: {}".format(base_params,
                                                                                                        keras_preds,
                                                                                                        coreml_preds))
                    numerical_failiure += 1
                    numerical_err_models.append(base_params)
            i += 1

        self.assertEquals(shape_err_models, [], msg='Shape error models {}'.format(shape_err_models))
        self.assertEquals(numerical_err_models, [], msg='Numerical error models {}\n'
                                                        'Total numerical failiures: {}/{}\n'.format(
            numerical_err_models,
            numerical_failiure, i)
                          )

    @unittest.skipIf(not HAS_KERAS_TF, 'Missing keras 1. Skipping test.')
    @pytest.mark.keras1
    @pytest.mark.slow
    def test_kers1_rnn_layer_stress(self):
        self._test_rnn_layer(keras_major_version=1)

    @unittest.skipIf(not HAS_KERAS_TF, 'Missing keras 1. Skipping test.')
    @pytest.mark.keras1
    def test_keras1_rnn_layer(self):
        self._test_rnn_layer(keras_major_version=1, limit=10)

    @unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras 2. Skipping test.')
    @pytest.mark.keras2
    @pytest.mark.slow
    def test_keras2_rnn_layer_stress(self):
        self._test_rnn_layer(keras_major_version=2)

    @unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras 2. Skipping test.')
    @pytest.mark.keras2
    def test_keras2_rnn_layer(self):
        self._test_rnn_layer(keras_major_version=2, limit=10)

class LSTMLayer(RecurrentLayerTest):
    """
    Class for testing single RNN layer
    """

    def setUp(self):
        super(LSTMLayer, self).setUp()
        self.lstm_params_dict = dict(
            inner_activation=['tanh', 'linear', 'sigmoid', 'hard_sigmoid', 'relu'],
            bidirectional=[True, False],
        )
        self.lstm_layer_params = list(itertools.product(*self.lstm_params_dict.values()))

    def _test_lstm_layer(self, keras_major_version, limit=None):
        i = 0
        params_keys = list(self.params_dict.keys())
        numerical_err_models = []
        shape_err_models = []
        numerical_failiure = 0
        params = list(itertools.product(self.base_layer_params, self.lstm_layer_params))
        np.random.shuffle(params)

        params = [param for param in params if valid_params(dict(zip(self.params_dict.keys(), param[0])))]
        for base_params, lstm_params in params[:limit]:
            base_params = dict(zip(self.params_dict.keys(), base_params))
            lstm_params = dict(zip(self.lstm_params_dict.keys(), lstm_params))
            model = Sequential()
            unroll = base_params['unroll']
            if base_params['input_dims'][1] == 1 and unroll == True:
                unroll = False
            if lstm_params['bidirectional'] is True:
                if keras_major_version == 2:
                    model.add(
                        Bidirectional(
                            LSTM(
                                base_params['output_dim'],
                                activation=base_params['activation'],
                                recurrent_activation=lstm_params['inner_activation'],
                                return_sequences=base_params['return_sequences'],
                                go_backwards=False,
                                unroll=unroll,
                            ),
                            input_shape=(base_params['input_dims'][1], base_params['input_dims'][2]),

                        )
                    )
                else:
                    model.add(
                        Bidirectional(
                            LSTM(
                                base_params['output_dim'],
                                activation=base_params['activation'],
                                inner_activation=lstm_params['inner_activation'],
                                return_sequences=base_params['return_sequences'],
                                go_backwards=False,
                                unroll=unroll,
                            ),
                            input_shape=(base_params['input_dims'][1], base_params['input_dims'][2]),

                        )
                    )
            else:
                if keras_major_version == 2:
                    model.add(
                        LSTM(
                            base_params['output_dim'],
                            input_shape=(base_params['input_dims'][1], base_params['input_dims'][2]),
                            activation=base_params['activation'],
                            recurrent_activation=lstm_params['inner_activation'],
                            return_sequences=base_params['return_sequences'],
                            go_backwards=base_params['go_backwards'],
                            unroll=unroll,
                        )
                    )
                else:
                    model.add(
                        LSTM(
                            base_params['output_dim'],
                            input_shape=(base_params['input_dims'][1], base_params['input_dims'][2]),
                            activation=base_params['activation'],
                            inner_activation=lstm_params['inner_activation'],
                            return_sequences=base_params['return_sequences'],
                            go_backwards=base_params['go_backwards'],
                            unroll=unroll,
                        )
                    )
            mlkitmodel = get_mlkit_model_from_path(model)
            input_data = generate_input(base_params['input_dims'][0], base_params['input_dims'][1],
                                        base_params['input_dims'][2])

            activations_to_test_with_numpy = {'linear', 'relu'}
            if base_params['activation'] in activations_to_test_with_numpy or lstm_params[
                'inner_activation'] in activations_to_test_with_numpy:
                if lstm_params['bidirectional']:
                    keras_preds = get_numpy_prediction_bilstm(model, input_data).flatten()
                else:
                    keras_preds = get_numpy_prediction_unilstm(model, input_data).flatten()
            else:
                keras_preds = model.predict(input_data).flatten()

            if macos_version() >= (10, 13):
                input_data = np.transpose(input_data, [1, 0, 2])
                coreml_preds = mlkitmodel.predict({'data': input_data})['output'].flatten()

                if K.tensorflow_backend._SESSION:
                    import tensorflow as tf
                    tf.reset_default_graph()
                    K.tensorflow_backend._SESSION.close()
                    K.tensorflow_backend._SESSION = None

                try:
                    self.assertEquals(coreml_preds.shape, keras_preds.shape)
                except AssertionError:
                    print(
                    "Shape error:\n base_params: {}\n\n lstm_params: {}\n\n keras_preds.shape: {}\n\n coreml_preds.shape: {}".format(
                        base_params, lstm_params, keras_preds.shape, coreml_preds.shape))
                    shape_err_models.append(base_params)
                    i += 1
                    continue
                try:
                    max_denominator = np.maximum(np.maximum(np.abs(coreml_preds), np.abs(keras_preds)), 1.0)
                    relative_error = coreml_preds / max_denominator - keras_preds / max_denominator
                    for i in range(len(relative_error)):
                        self.assertLessEqual(relative_error[i], 0.01)
                except AssertionError:
                    print(
                    "Assertion error:\n base_params: {}\n lstm_params: {}\n\n keras_preds: {}\n\n coreml_preds: {}\n\n\n keras_preds: {}\n\n\n coreml_preds: {}\n".format(
                        base_params,
                        lstm_params,
                        keras_preds / max_denominator,
                        coreml_preds / max_denominator,
                        keras_preds,
                        coreml_preds))
                    numerical_failiure += 1
                    numerical_err_models.append(base_params)
            i += 1

        self.assertEquals(shape_err_models, [], msg='Shape error models {}'.format(shape_err_models))
        self.assertEquals(numerical_err_models, [], msg='Numerical error models {}'.format(numerical_err_models))

    @unittest.skipIf(not HAS_KERAS_TF, 'Missing keras 1. Skipping test.')
    @pytest.mark.keras1
    @pytest.mark.slow
    def test_keras_lstm_layer_stress(self):
        self._test_lstm_layer(keras_major_version=1)

    @unittest.skipIf(not HAS_KERAS_TF, 'Missing keras 1. Skipping test.')
    @pytest.mark.keras1
    def test_keras_lstm_layer(self):
        self._test_lstm_layer(keras_major_version=1, limit=10)

    @unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras 2. Skipping test.')
    @pytest.mark.keras2
    @pytest.mark.slow
    def test_keras2_lstm_layer_stress(self):
        self._test_lstm_layer(keras_major_version=2)

    @unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras 2. Skipping test.')
    @pytest.mark.keras2
    def test_keras2_lstm_layer(self):
        self._test_lstm_layer(keras_major_version=2, limit=10)

class GRULayer(RecurrentLayerTest):
    """
    Class for testing GRU layer
    """

    def setUp(self):
        super(GRULayer, self).setUp()
        self.gru_params_dict = dict(
            inner_activation=['tanh', 'linear', 'sigmoid', 'hard_sigmoid', 'relu']
        )
        self.gru_layer_params = list(itertools.product(*self.gru_params_dict.values()))

    def _test_gru_layer(self, keras_major_version, limit=None):
        i = 0
        numerical_err_models = []
        shape_err_models = []
        numerical_failiure = 0
        params = list(itertools.product(self.base_layer_params, self.gru_layer_params))
        np.random.shuffle(params)
        params = [param for param in params if valid_params(dict(zip(self.params_dict.keys(), param[0])))]
        for base_params, gru_params in params[:limit]:
            base_params = dict(zip(self.params_dict.keys(), base_params))
            gru_params = dict(zip(self.gru_params_dict.keys(), gru_params))
            model = Sequential()
            unroll = base_params['unroll']
            if base_params['input_dims'][1] == 1 and unroll == True:
                unroll = False
            if keras_major_version == 2:
                model.add(
                    GRU(
                        base_params['output_dim'],
                        input_shape=(base_params['input_dims'][1],base_params['input_dims'][2]),
                        activation=base_params['activation'],
                        recurrent_activation=gru_params['inner_activation'],
                        return_sequences=base_params['return_sequences'],
                        go_backwards=base_params['go_backwards'],
                        unroll=unroll,
                    )
                )
            else:
                model.add(
                    GRU(
                        base_params['output_dim'],
                        input_length=base_params['input_dims'][1],
                        input_dim=base_params['input_dims'][2],
                        activation=base_params['activation'],
                        inner_activation=gru_params['inner_activation'],
                        return_sequences=base_params['return_sequences'],
                        go_backwards=base_params['go_backwards'],
                        unroll=unroll,
                    )
                )
            model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])
            mlkitmodel = get_mlkit_model_from_path(model)
            input_data = generate_input(base_params['input_dims'][0], base_params['input_dims'][1],
                                        base_params['input_dims'][2])

            activations_to_test_with_numpy = {'linear', 'relu'}
            if base_params['activation'] in activations_to_test_with_numpy or gru_params[
                'inner_activation'] in activations_to_test_with_numpy:
                keras_preds = get_numpy_prediction_gru(model, input_data).flatten()
            else:
                keras_preds = model.predict(input_data).flatten()

            if macos_version() >= (10, 13):
                input_data = np.transpose(input_data, [1, 0, 2])
                coreml_preds = mlkitmodel.predict({'data': input_data})['output'].flatten()
                if K.tensorflow_backend._SESSION:
                    import tensorflow as tf
                    tf.reset_default_graph()
                    K.tensorflow_backend._SESSION.close()
                    K.tensorflow_backend._SESSION = None
                try:
                    self.assertEquals(coreml_preds.shape, keras_preds.shape)
                except AssertionError:
                    print(
                    "Shape error:\nbase_params: {}\n gru_params: {}\nkeras_preds.shape: {}\ncoreml_preds.shape: {}".format(
                        base_params, gru_params, keras_preds.shape, coreml_preds.shape))
                    shape_err_models.append(base_params)
                    i += 1
                    continue
                try:
                    max_denominator = np.maximum(np.maximum(np.abs(coreml_preds), np.abs(keras_preds)), 1.0)
                    relative_error = coreml_preds / max_denominator - keras_preds / max_denominator
                    for i in range(len(relative_error)):
                        self.assertLessEqual(relative_error[i], 0.01)
                except AssertionError:
                    print(
                    "===============Assertion error:\n base_params: {}\n gru_params: {}\n\n keras_preds: {}\n\n coreml_preds: {}\n\n\n keras_preds: {}\n\n\n coreml_preds: {}\n".format(
                        base_params,
                        gru_params,
                        keras_preds / max_denominator,
                        coreml_preds / max_denominator,
                        keras_preds,
                        coreml_preds))
                    numerical_failiure += 1
                    numerical_err_models.append(base_params)
            i += 1

        self.assertEquals(shape_err_models, [], msg='Shape error models {}'.format(shape_err_models))
        self.assertEquals(numerical_err_models, [], msg='Numerical error models {}'.format(numerical_err_models))

    @unittest.skipIf(not HAS_KERAS_TF, 'Missing keras 1. Skipping test.')
    @pytest.mark.keras1
    @pytest.mark.slow
    def test_keras1_test_gru_layer_stress(self):
        self._test_gru_layer(keras_major_version=1)

    @unittest.skipIf(not HAS_KERAS_TF, 'Missing keras 1. Skipping test.')
    @pytest.mark.keras1
    def test_keras1_test_gru_layer(self):
        self._test_gru_layer(keras_major_version=1, limit=10)

    @unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras 2. Skipping test.')
    @pytest.mark.keras2
    @pytest.mark.slow
    def test_keras2_test_gru_layer_stress(self):
        self._test_gru_layer(keras_major_version=2)

    @unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras 2. Skipping test.')
    @pytest.mark.keras2
    def test_keras2_test_gru_layer(self):
        self._test_gru_layer(keras_major_version=2, limit=10)


class LSTMStacked(unittest.TestCase):
    """
    Class for testing LSTMStacked
    """

    def setUp(self):
        self.params_dict = dict(
            input_dims=[[1, 1, 1], [1, 2, 5], [1, 5, 10]],
            output_dim=[1, 5, 10, 20],
            stateful=[False],
            go_backwards=[False],
            unroll=[True],
            return_sequences=[True],
            top_return_sequences=[True, False],
            activation=['tanh', 'sigmoid', 'hard_sigmoid'],
            number_of_layers=[1, 2, 3]
        )
        self.base_layer_params = list(itertools.product(*self.params_dict.values()))

    def _test_lstm_stacked(self, keras_major_version, limit=None):
        numerical_err_models = []
        shape_err_models = []
        numerical_failiure = 0
        params = copy(self.base_layer_params)
        np.random.shuffle(params)
        i = 0
        params = [param for param in params if valid_params(dict(zip(self.params_dict.keys(), param)))]
        for base_params in params[:limit]:
            base_params = dict(zip(self.params_dict.keys(), base_params))
            model = Sequential()
            unroll = base_params['unroll']
            if base_params['input_dims'][1] == 1 and unroll == True:
                unroll = False
            settings = dict(
                activation=base_params['activation'],
                return_sequences=True,
                go_backwards=base_params['go_backwards'],
                unroll=unroll,
            )
            if keras_major_version == 2:
                model.add(LSTM(base_params['output_dim'],
                               input_shape=base_params['input_dims'][1:],
                               recurrent_activation='sigmoid',
                               **settings))
                for idx in range(0, base_params['number_of_layers']):
                    model.add(LSTM(base_params['output_dim'],
                                   input_shape=(base_params['input_dims'][1], base_params['output_dim']),
                                   return_sequences=True,
                                   activation='tanh',
                                   recurrent_activation='sigmoid',
                                   ))
                model.add(LSTM(10, input_shape=(base_params['input_dims'][1], base_params['output_dim']),
                               return_sequences=base_params['top_return_sequences'], activation='sigmoid'))

            else:
                model.add(LSTM(output_dim=base_params['output_dim'],
                               input_length=base_params['input_dims'][1],
                               input_dim=base_params['input_dims'][2],
                               inner_activation='sigmoid',
                               **settings))
                for idx in range(0, base_params['number_of_layers']):
                    model.add(LSTM(output_dim=base_params["output_dim"],
                                   return_sequences=True,
                                   activation='tanh',
                                   inner_activation='sigmoid',
                                   ))
                model.add(LSTM(output_dim=10, return_sequences=base_params['top_return_sequences'], activation='sigmoid'))
            mlkitmodel = get_mlkit_model_from_path(model)
            input_data = generate_input(base_params['input_dims'][0], base_params['input_dims'][1],
                                        base_params['input_dims'][2])
            if macos_version() >= (10, 13):
                keras_preds = model.predict(input_data).flatten()
                input_data = np.transpose(input_data, [1, 0, 2])
                coreml_preds = mlkitmodel.predict({'data': input_data})['output'].flatten()
                import tensorflow as tf
                tf.reset_default_graph()
                K.tensorflow_backend._SESSION.close()
                K.tensorflow_backend._SESSION = None
                try:
                    self.assertEquals(coreml_preds.shape, keras_preds.shape)
                except AssertionError:
                    print("Shape error:\nbase_params: {}\nkeras_preds.shape: {}\ncoreml_preds.shape: {}".format(
                        base_params, keras_preds.shape, coreml_preds.shape))
                    shape_err_models.append(base_params)
                    i += 1
                    continue
                try:
                    max_denominator = np.maximum(np.maximum(np.abs(coreml_preds), np.abs(keras_preds)), 1.0)
                    relative_error = coreml_preds / max_denominator - keras_preds / max_denominator
                    for i in range(len(relative_error)):
                        self.assertLessEqual(relative_error[i], 0.01)
                except AssertionError:
                    print("Assertion error:\nbase_params: {}\nkeras_preds: {}\ncoreml_preds: {}".format(base_params,
                                                                                                        keras_preds,
                                                                                                        coreml_preds))
                    numerical_failiure += 1
                    numerical_err_models.append(base_params)
            i += 1
        self.assertEquals(shape_err_models, [], msg='Shape error models {}'.format(shape_err_models))
        self.assertEquals(numerical_err_models, [], msg='Numerical error models {}'.format(numerical_err_models))

    @unittest.skipIf(not HAS_KERAS_TF, 'Missing keras 1. Skipping test.')
    @pytest.mark.keras1
    @pytest.mark.slow
    def test_keras1_lstm_stacked_stress(self):
        self._test_lstm_stacked(keras_major_version=1)

    @unittest.skipIf(not HAS_KERAS_TF, 'Missing keras 1. Skipping test.')
    @pytest.mark.keras1
    def test_keras1_lstm_stacked(self):
        self._test_lstm_stacked(keras_major_version=1, limit=10)

    @unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras 2. Skipping test.')
    @pytest.mark.keras2
    @pytest.mark.slow
    def test_keras2_lstm_stacked_stress(self):
        self._test_lstm_stacked(keras_major_version=2)

    @unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras 2. Skipping test.')
    @pytest.mark.keras2
    def test_keras2_lstm_stacked(self):
        self._test_lstm_stacked(keras_major_version=2, limit=10)


class DifferentIOModelsTypes(unittest.TestCase):
    """
    Class for testing different I/O combinations for LSTMS
    """

    def _test_one_to_many(self, keras_major_version):
        params = dict(
            input_dims=[1, 10], activation='tanh',
            return_sequences=False, output_dim=3
        ),
        number_of_times = 4
        model = Sequential()
        model.add(RepeatVector(number_of_times, input_shape=(10,)))

        if keras_major_version == 2:
            model.add(LSTM(params[0]['output_dim'],
                           input_shape=params[0]['input_dims'],
                           activation=params[0]['activation'],
                           recurrent_activation='sigmoid',
                           return_sequences=True,
                           ))
        else:
            model.add(LSTM(output_dim=params[0]['output_dim'],
                           activation=params[0]['activation'],
                           inner_activation='sigmoid',
                           return_sequences=True,
                           ))
        relative_error, keras_preds, coreml_preds = simple_model_eval(params, model)
        # print relative_error, '\n', keras_preds, '\n', coreml_preds, '\n'
        for i in range(len(relative_error)):
            self.assertLessEqual(relative_error[i], 0.01)

    def _test_many_to_one(self, keras_major_version):
        params = dict(
            input_dims=[1, 10, 5], go_backwards=False, activation='tanh',  # fails with hard_sigmoid
            stateful=False, unroll=False, return_sequences=False, output_dim=1
        ),
        model = Sequential()
        if keras_major_version == 2:
            model.add(LSTM(params[0]['output_dim'],
                           input_shape=params[0]['input_dims'][1:],
                           activation=params[0]['activation'],
                           recurrent_activation='sigmoid',
                           ))
        else:
            model.add(LSTM(output_dim=params[0]['output_dim'],
                           input_shape=params[0]['input_dims'][1:],
                           activation=params[0]['activation'],
                           inner_activation='sigmoid',
                           ))
        relative_error, keras_preds, coreml_preds = simple_model_eval(params, model)
        # print relative_error, '\n', keras_preds, '\n', coreml_preds, '\n'
        for i in range(len(relative_error)):
            self.assertLessEqual(relative_error[i], 0.01)

    def _test_many_to_many(self, keras_major_version):
        params = dict(
            input_dims=[1, 10, 5], go_backwards=False, activation='tanh',  # fails with hard_sigmoid
            stateful=False, unroll=False, return_sequences=True, output_dim=1
        ),
        model = Sequential()
        if keras_major_version == 2:
            model.add(LSTM(params[0]['output_dim'],
                           input_shape=params[0]['input_dims'][1:],
                           activation=params[0]['activation'],
                           recurrent_activation='sigmoid',
                           return_sequences=True,
                           ))
        else:
            model.add(LSTM(output_dim=params[0]['output_dim'],
                           input_shape=params[0]['input_dims'][1:],
                           activation=params[0]['activation'],
                           inner_activation='sigmoid',
                           return_sequences=True,
                           ))
        relative_error, keras_preds, coreml_preds = simple_model_eval(params, model)
        # print relative_error, '\n', keras_preds, '\n', coreml_preds, '\n'
        for i in range(len(relative_error)):
            self.assertLessEqual(relative_error[i], 0.01)

    @unittest.skipIf(not HAS_KERAS_TF, 'Missing keras 1. Skipping test.')
    @pytest.mark.keras1
    def test_keras1_test_one_to_many(self):
        self._test_one_to_many(keras_major_version=1)

    @unittest.skipIf(not HAS_KERAS_TF, 'Missing keras 1. Skipping test.')
    @pytest.mark.keras1
    def test_keras1_test_many_to_one(self):
        self._test_many_to_one(keras_major_version=1)

    @unittest.skipIf(not HAS_KERAS_TF, 'Missing keras 1. Skipping test.')
    @pytest.mark.keras1
    def test_keras1_many_to_many(self):
        self._test_many_to_many(keras_major_version=1)

    @unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras 2. Skipping test.')
    @pytest.mark.keras2
    def test_keras2_test_one_to_many(self):
        self._test_one_to_many(keras_major_version=2)

    @unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras 2. Skipping test.')
    @pytest.mark.keras2
    def test_keras2_test_many_to_one(self):
        self._test_many_to_one(keras_major_version=2)

    @unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras 2. Skipping test.')
    @pytest.mark.keras2
    def test_keras2_many_to_many(self):
        self._test_many_to_many(keras_major_version=2)


@unittest.skipIf(not HAS_KERAS2_TF, 'Missing keras 2. Skipping test.')
@pytest.mark.keras2
class InitialStateRecurrentModels(unittest.TestCase):
    """
    This test class sets initial states to the recurrent nodes
    and then tests if the state is passed in Espresso
    """
    @unittest.skip("failing - TODO reenable when it passes consistently")
    def test_initial_state_GRU(self):
        data = np.random.rand(1, 1, 2)

        model = keras.models.Sequential()
        model.add(keras.layers.GRU(5, input_shape=(1, 2), batch_input_shape=[1, 1, 2], stateful=True))
        model.get_layer(index=1).reset_states()

        coreml_model = keras_converter.convert(model=model, input_names='data', output_names='output')
        if macos_version() >= (10, 13):
            keras_output_1 = model.predict(data)
            coreml_full_output_1 = coreml_model.predict({'data': data})
            coreml_output_1 = coreml_full_output_1['output']
            coreml_output_1 = np.expand_dims(coreml_output_1, 1)

            np.testing.assert_array_almost_equal(coreml_output_1.T, keras_output_1)

        hidden_state = (np.random.rand(1, 5))
        model.get_layer(index=1).reset_states(hidden_state)
        coreml_model = keras_converter.convert(model=model, input_names='data', output_names='output')
        spec = coreml_model.get_spec()
        if macos_version() >= (10, 13):
            keras_output_2 = model.predict(data)
            coreml_full_output_2 = coreml_model.predict({'data': data, spec.description.input[1].name: hidden_state[0]})
            coreml_output_2 = coreml_full_output_2['output']
            coreml_output_2 = np.expand_dims(coreml_output_2, 1)
            np.testing.assert_array_almost_equal(coreml_output_2.T, keras_output_2)

    def test_initial_state_SimpleRNN(self):
        data = np.random.rand(1, 1, 2)
        model = keras.models.Sequential()
        model.add(keras.layers.SimpleRNN(5, input_shape=(1, 2), batch_input_shape=[1, 1, 2], stateful=True))
        model.get_layer(index=1).reset_states()
        coreml_model = keras_converter.convert(model=model, input_names='data', output_names='output')
        if macos_version() >= (10, 13):
            keras_output_1 = model.predict(data)
            coreml_full_output_1 = coreml_model.predict({'data': data})
            coreml_output_1 = coreml_full_output_1['output']
            coreml_output_1 = np.expand_dims(coreml_output_1, 1)
            np.testing.assert_array_almost_equal(coreml_output_1.T, keras_output_1)

        hidden_state = np.random.rand(1, 5)
        model.get_layer(index=1).reset_states(hidden_state)
        coreml_model = keras_converter.convert(model=model, input_names='data', output_names='output')
        spec = coreml_model.get_spec()
        if macos_version() >= (10, 13):
            keras_output_2 = model.predict(data)
            coreml_full_output_2 = coreml_model.predict({'data': data, spec.description.input[1].name: hidden_state[0]})
            coreml_output_2 = coreml_full_output_2['output']
            coreml_output_2 = np.expand_dims(coreml_output_2, 1)
            np.testing.assert_array_almost_equal(coreml_output_2.T, keras_output_2)

    def test_initial_state_LSTM(self):
        data = np.random.rand(1, 1, 2)

        model = keras.models.Sequential()
        model.add(keras.layers.LSTM(5, input_shape=(1, 2), batch_input_shape=[1, 1, 2], stateful=True))
        model.get_layer(index=1).reset_states()

        if macos_version() >= (10, 13):
            coreml_model = keras_converter.convert(model=model, input_names='data', output_names='output')

            keras_output_1 = model.predict(data)
            coreml_full_output_1 = coreml_model.predict({'data': data})
            coreml_output_1 = coreml_full_output_1['output']
            coreml_output_1 = np.expand_dims(coreml_output_1, 1)

            np.testing.assert_array_almost_equal(coreml_output_1.T, keras_output_1)

        hidden_state = (np.random.rand(1, 5), np.random.rand(1, 5))
        model.get_layer(index=1).reset_states(hidden_state)

        coreml_model = keras_converter.convert(model=model, input_names='data', output_names='output')
        spec = coreml_model.get_spec()

        if macos_version() >= (10, 13):
            keras_output_2 = model.predict(data)
            coreml_full_output_2 = coreml_model.predict(
                {'data': data, spec.description.input[1].name: hidden_state[0][0],
                 spec.description.input[2].name: hidden_state[1][0]})
            coreml_output_2 = coreml_full_output_2['output']
            coreml_output_2 = np.expand_dims(coreml_output_2, 1)

            np.testing.assert_array_almost_equal(coreml_output_2.T, keras_output_2)
