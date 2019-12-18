# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from . import _utils
import keras
import numpy as np


def _get_recurrent_activation_name_from_keras(activation):
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
        raise NotImplementedError(
            'activation %s not supported for Recurrent layer.' % activation)

    return activation_str

def _get_activation_name_from_keras_layer(keras_layer):

    if isinstance(keras_layer, keras.layers.advanced_activations.LeakyReLU):
        non_linearity = 'LEAKYRELU'
    elif isinstance(keras_layer, keras.layers.advanced_activations.PReLU):
        non_linearity = 'PRELU'
    elif isinstance(keras_layer, keras.layers.advanced_activations.ELU):
        non_linearity = 'ELU'
    elif isinstance(keras_layer, keras.layers.advanced_activations.ParametricSoftplus):
        non_linearity = 'PARAMETRICSOFTPLUS'
    elif isinstance(keras_layer, keras.layers.advanced_activations.ThresholdedReLU):
        non_linearity = 'THRESHOLDEDRELU'
    else:
        import six
        if six.PY2:
            act_name = keras_layer.activation.func_name
        else:
            act_name = keras_layer.activation.__name__

        if act_name == 'softmax':
            non_linearity = 'SOFTMAX'
        elif act_name == 'sigmoid':
            non_linearity = 'SIGMOID'
        elif act_name == 'tanh':
            non_linearity = 'TANH'
        elif act_name == 'relu':
            non_linearity = 'RELU'
        elif act_name == 'softplus':
            non_linearity = 'SOFTPLUS'
        elif act_name == 'softsign':
            non_linearity = 'SOFTSIGN'
        elif act_name == 'hard_sigmoid':
            non_linearity = 'SIGMOID_HARD'
        elif act_name == 'linear':
            non_linearity = 'LINEAR'
        else:
            _utils.raise_error_unsupported_categorical_option('activation',
                                                            act_name, 'Dense', ##
                    keras_layer.name)

    return non_linearity

def _get_elementwise_name_from_keras_layer(keras_layer):
    """
    Get the keras layer name from the activation name.
    """
    mode = keras_layer.mode
    if mode == 'sum':
        return 'ADD'
    elif mode == 'mul':
        return 'MULTIPLY'
    elif mode == 'concat':
        if len(keras_layer.input_shape[0]) == 3 and (keras_layer.concat_axis == 1
                                                     or keras_layer.concat_axis == -2):
            return 'SEQUENCE_CONCAT'
        elif len(keras_layer.input_shape[0]) == 4 and (keras_layer.concat_axis == 3
                                                       or keras_layer.concat_axis == -1):
            return 'CONCAT'
        elif len(keras_layer.input_shape[0]) == 2 and (keras_layer.concat_axis == 1
                                                       or keras_layer.concat_axis == -1):
            return 'CONCAT'
        else:
            option = "input_shape = %s concat_axis = %s" \
                     % (str(keras_layer.input_shape[0]), str(keras_layer.concat_axis))
            _utils.raise_error_unsupported_option(option, mode, keras_layer.name)
    elif mode == 'cos':
        if len(keras_layer.input_shape[0]) == 2: 
            return 'COS'
        else:
            option = "input_shape = %s" % (str(keras_layer.input_shape[0]))
            _utils.raise_error_unsupported_option(option, mode, keras_layer.name)
    elif mode == 'dot':
        if len(keras_layer.input_shape[0]) == 2:  
            return 'DOT'
        else:
            option = "input_shape = %s" % (str(keras_layer.input_shape[0]))
            _utils.raise_error_unsupported_option(option, mode, keras_layer.name)
    elif mode == 'max':
        return 'MAX'
    elif mode == 'ave':
        return 'AVE'
    else:
        _utils.raise_error_unsupported_categorical_option('mode', mode, 'Merge',
                keras_layer.name)

def _same_elements_per_channel(x):
    """
    Test if a 3D (H,W,C) matrix x has the same element in each (H,W) matrix for each channel
    """
    eps = 1e-5
    dims = x.shape
    for c in range(dims[-1]): 
        xc = x[:,:,c].flatten()
        if not np.all(np.absolute(xc - xc[0]) < eps):
            return False
    return True

def convert_dense(builder, layer, input_names, output_names, keras_layer):
    """Convert a dense layer from keras to coreml.

    Parameters
    keras_layer: layer
    ----------
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])

    has_bias = keras_layer.bias
    # Get the weights from keras
    W = keras_layer.get_weights ()[0].T
    Wb = keras_layer.get_weights ()[1].T if has_bias else None

    builder.add_inner_product(name = layer,
            W = W,
            b = Wb,
            input_channels = keras_layer.input_dim,
            output_channels = keras_layer.output_dim,
            has_bias = has_bias,
            input_name = input_name,
            output_name = output_name)

def convert_activation(builder, layer, input_names, output_names, keras_layer):
    """Convert an activation layer from keras to coreml.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])
    non_linearity = _get_activation_name_from_keras_layer(keras_layer)

    # Add a non-linearity layer
    if non_linearity == 'SOFTMAX':
        builder.add_softmax(name = layer, input_name = input_name,
                output_name = output_name)
        return

    params = None
    if non_linearity == 'LEAKYRELU':
        params = [keras_layer.alpha]

    elif non_linearity == 'PRELU':
        # In Keras 1.2  PReLU layer's weights are stored as a 
        # backend tensor, not a numpy array as it claims in documentation. 
        shared_axes = list(keras_layer.shared_axes)
        if not (shared_axes == [1,2,3] or shared_axes == [1,2]):
            _utils.raise_error_unsupported_scenario("Shared axis not being [1,2,3] "
                                                    "or [1,2]", 'parametric_relu', layer)
        params = keras.backend.eval(keras_layer.weights[0])
    elif non_linearity == 'ELU':
        params = keras_layer.alpha

    elif non_linearity == 'PARAMETRICSOFTPLUS':
        # In Keras 1.2  Parametric Softplus layer's weights are stored as a 
        # backend tensor, not a numpy array as it claims in documentation.
        alphas = keras.backend.eval(keras_layer.weights[0])
        betas = keras.backend.eval(keras_layer.weights[1])

        if len(alphas.shape) == 3:  # (H,W,C)
            if not (_same_elements_per_channel(alphas) and
                    _same_elements_per_channel(betas)):
                _utils.raise_error_unsupported_scenario("Different parameter values",
                                                        'parametric_softplus', layer)
            alphas = alphas[0,0,:]
            betas = betas[0,0,:]
        params = [alphas, betas]
                
    elif non_linearity == 'THRESHOLDEDRELU':
        params = keras_layer.theta
    else:
        pass # do nothing to parameters
    builder.add_activation(name = layer, 
            non_linearity = non_linearity,
            input_name = input_name, output_name = output_name, 
            params = params)

def convert_merge(builder, layer, input_names, output_names, keras_layer):
    """Convert concat layer from keras to coreml.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    # Get input and output names
    output_name = output_names[0]

    mode = _get_elementwise_name_from_keras_layer(keras_layer)
    builder.add_elementwise(name = layer, input_names = input_names,
            output_name = output_name, mode = mode)

def convert_pooling(builder, layer, input_names, output_names, keras_layer):
    """Convert pooling layer from keras to coreml.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])

    # Pooling layer type
    if isinstance(keras_layer, keras.layers.convolutional.MaxPooling2D) or \
        isinstance(keras_layer, keras.layers.convolutional.MaxPooling1D) or \
        isinstance(keras_layer, keras.layers.pooling.GlobalMaxPooling2D) or \
        isinstance(keras_layer, keras.layers.pooling.GlobalMaxPooling1D): 
        layer_type_str = 'MAX'
    elif isinstance(keras_layer, keras.layers.convolutional.AveragePooling2D) or \
        isinstance(keras_layer, keras.layers.convolutional.AveragePooling1D) or \
        isinstance(keras_layer, keras.layers.pooling.GlobalAveragePooling2D) or \
        isinstance(keras_layer, keras.layers.pooling.GlobalAveragePooling1D): 
        layer_type_str = 'AVERAGE'
    else:
        raise TypeError("Pooling type %s not supported" % keras_layer)

    # if it's global, set the global flag
    if isinstance(keras_layer, keras.layers.pooling.GlobalMaxPooling2D) or \
        isinstance(keras_layer, keras.layers.pooling.GlobalAveragePooling2D): 
        # 2D global pooling
        global_pooling = True
        height, width = (0, 0)
        stride_height, stride_width = (0,0)
        padding_type = 'VALID'
    elif isinstance(keras_layer, keras.layers.pooling.GlobalMaxPooling1D) or \
        isinstance(keras_layer, keras.layers.pooling.GlobalAveragePooling1D):
        # 1D global pooling: 1D global pooling seems problematic, 
        # use this work-around
        global_pooling = False
        _, width, channels = keras_layer.input_shape
        height = 1
        stride_height, stride_width = height, width
        padding_type = 'VALID'
    else:
        global_pooling = False
        # Set pool sizes and strides
        # 1D cases: 
        if isinstance(keras_layer, keras.layers.convolutional.MaxPooling1D) or \
            isinstance(keras_layer, keras.layers.pooling.GlobalMaxPooling1D) or \
            isinstance(keras_layer, keras.layers.convolutional.AveragePooling1D) or \
            isinstance(keras_layer, keras.layers.pooling.GlobalAveragePooling1D): 
            height, width = 1, keras_layer.pool_length
            if keras_layer.stride is not None: 
                stride_height, stride_width = 1, keras_layer.stride
            else:
                stride_height, stride_width = 1, keras_layer.pool_length
        # 2D cases: 
        else:        
            height, width = keras_layer.pool_size
            if keras_layer.strides is None:
                stride_height, stride_width = height, width
            else:
                stride_height, stride_width = keras_layer.strides

        # Padding
        border_mode = keras_layer.border_mode
        if keras_layer.border_mode == 'valid':
            padding_type = 'VALID'
        elif keras_layer.border_mode == 'same':
            padding_type = 'SAME'
        else:
            raise TypeError("Border mode %s not supported" % border_mode)
    
    builder.add_pooling(name = layer,
        height = height,
        width = width,
        stride_height = stride_height,
        stride_width = stride_width,
        layer_type = layer_type_str,
        padding_type = padding_type,
        input_name = input_name,
        output_name = output_name, 
        exclude_pad_area = True, 
        is_global = global_pooling)

def convert_padding(builder, layer, input_names, output_names, keras_layer):
    """Convert padding layer from keras to coreml.
    Keras only supports zero padding at this time. 
    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])
    
    if isinstance(keras_layer, keras.layers.convolutional.ZeroPadding1D):
        left, right = keras_layer.padding
        top, bottom = (0, 0)
    else: # 2D
        top, left = keras_layer.padding
        bottom, right = keras_layer.padding

    # Now add the layer
    builder.add_padding(name = layer,
        left = left, right=right, top=top, bottom=bottom, value = 0, 
        input_name = input_name, output_name=output_name
        )

def convert_cropping(builder, layer, input_names, output_names, keras_layer):
    """Convert padding layer from keras to coreml.
    Keras only supports zero padding at this time. 
    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])
    
    if isinstance(keras_layer, keras.layers.convolutional.Cropping1D):
        left, right = keras_layer.cropping
        top, bottom = (0, 0)
    else: # 2D
        left, right = keras_layer.cropping[0]
        top, bottom = keras_layer.cropping[1]
    
    # Now add the layer
    builder.add_crop(name = layer,
        left = left, right=right, top=top, bottom=bottom, offset = [0,0], 
        input_names = [input_name], output_name=output_name
        )

def convert_reshape(builder, layer, input_names, output_names, keras_layer):
    
    input_name, output_name = (input_names[0], output_names[0])
    
    input_shape = keras_layer.input_shape
    target_shape = keras_layer.target_shape
    
    def get_coreml_target_shape(target_shape):
        if len(target_shape) == 1: #(D,)
            coreml_shape = (1,target_shape[0],1,1)
        elif len(target_shape) == 2: #(S,D)
            coreml_shape = target_shape + (1,1)
        elif len(target_shape) == 3: #(H,W,C)
            coreml_shape = (1, target_shape[2], target_shape[0], target_shape[1])
        else:
            coreml_shape = None
        return coreml_shape
    
    def get_mode(input_shape, target_shape):
        in_shape = input_shape[1:]
        if len(in_shape) == 3 or len(target_shape) == 3:
                return 1
        else:
            return 0
    
    new_shape = get_coreml_target_shape(target_shape)
    if new_shape is not None: 
        mode = get_mode(input_shape, target_shape)
        builder.add_reshape(name = layer, input_name = input_name,
                            output_name=output_name,
                target_shape = new_shape, mode = mode)
    else: 
        _utils.raise_error_unsupported_categorical_option('input_shape',
                                                          str(input_shape),
                                                          'reshape', layer)

def convert_upsample(builder, layer, input_names, output_names, keras_layer):
    """Convert convolution layer from keras to coreml.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])

    if isinstance(keras_layer, keras.layers.convolutional.UpSampling1D):
        fh, fw = 1, keras_layer.length
    else: # 2D
        fh, fw = keras_layer.size

    builder.add_upsample(name = layer,
             scaling_factor_h = fh,
             scaling_factor_w = fw, 
             input_name = input_name,
             output_name = output_name)

def convert_convolution(builder, layer, input_names, output_names, keras_layer):
    """Convert convolution layer from keras to coreml.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])

    has_bias = keras_layer.bias
    is_deconv = isinstance(keras_layer, keras.layers.convolutional.Deconvolution2D)

    # Get the weights from keras.
    # Keras stores convolution weights as list of numpy arrays
    weightList = keras_layer.get_weights()
    output_shape = list(filter(None, keras_layer.output_shape))[:-1]

    # Parameter
    height, width, channels, n_filters = weightList[0].shape
    stride_height, stride_width = keras_layer.subsample

    # Weights and bias terms
    W = weightList[0]
    b = weightList[1] if has_bias else None

    # dilation factors
    dilation_factors = [1,1]
    if isinstance(keras_layer, keras.layers.convolutional.AtrousConvolution2D):
        dilation_factors = list(keras_layer.atrous_rate)

    builder.add_convolution(name = layer,
             kernel_channels = channels,
             output_channels = n_filters,
             height = height,
             width = width,
             stride_height = stride_height,
             stride_width = stride_width,
             border_mode = keras_layer.border_mode,
             groups = 1,
             W = W,
             b = b,
             has_bias = has_bias,
             is_deconv = is_deconv,
             output_shape = output_shape,
             input_name = input_name,
             output_name = output_name,
             dilation_factors = dilation_factors)

def convert_convolution1d(builder, layer, input_names, output_names, keras_layer):
    """Convert convolution layer from keras to coreml.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])

    has_bias = keras_layer.bias

    # Get the weights from keras.
    # Keras stores convolution weights as list of numpy arrays
    weightList = keras_layer.get_weights()
    output_shape = list(filter(None, keras_layer.output_shape))[:-1]

    # Parameter
    # weightList[0].shape = [kernel_length, input_length(time_step), input_dim, num_kernels]
    filter_length, input_length, input_dim, n_filters = weightList[0].shape
    stride_width = keras_layer.subsample[0]

    # Weights and bias terms
    W = weightList[0]
    b = weightList[1] if has_bias else None

    dilation_factors = [1,1]
    if isinstance(keras_layer, keras.layers.convolutional.AtrousConvolution1D):
        dilation_factors[-1] = keras_layer.atrous_rate

    builder.add_convolution(name = layer,
             kernel_channels = input_dim,
             output_channels = n_filters,
             height = 1,
             width = filter_length,
             stride_height = 1,
             stride_width = stride_width,
             border_mode = keras_layer.border_mode,
             groups = 1,
             W = W,
             b = b,
             has_bias = has_bias,
             is_deconv = False, 
             output_shape = output_shape,
             input_name = input_name,
             output_name = output_name, 
             dilation_factors = dilation_factors)    

def convert_lstm(builder, layer, input_names, output_names, keras_layer):
    """Convert an LSTM layer from keras to coreml.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    
    hidden_size = keras_layer.output_dim
    input_size = keras_layer.input_shape[-1]
    if keras_layer.consume_less not in ['cpu', 'gpu']:
        raise ValueError('Cannot convert Keras layer with consume_less = %s'
                         % keras_layer.consume_less)

    output_all = keras_layer.return_sequences
    reverse_input = keras_layer.go_backwards

    # Keras: I C F O; W_x, W_h, b
    # CoreML: I F O G; W_h and W_x are separated
    W_h, W_x, b = ([], [], [])
    if keras_layer.consume_less == 'cpu':
        W_h.append(keras_layer.get_weights()[1].T)
        W_h.append(keras_layer.get_weights()[7].T)
        W_h.append(keras_layer.get_weights()[10].T)
        W_h.append(keras_layer.get_weights()[4].T)

        W_x.append(keras_layer.get_weights()[0].T)
        W_x.append(keras_layer.get_weights()[6].T)
        W_x.append(keras_layer.get_weights()[9].T)
        W_x.append(keras_layer.get_weights()[3].T)

        b.append(keras_layer.get_weights()[2])
        b.append(keras_layer.get_weights()[8])
        b.append(keras_layer.get_weights()[11])
        b.append(keras_layer.get_weights()[5])
    else:
        keras_W_h = keras_layer.get_weights()[1].T
        W_h.append(keras_W_h[0 * hidden_size:][:hidden_size])
        W_h.append(keras_W_h[1 * hidden_size:][:hidden_size])
        W_h.append(keras_W_h[3 * hidden_size:][:hidden_size])
        W_h.append(keras_W_h[2 * hidden_size:][:hidden_size])

        keras_W_x = keras_layer.get_weights()[0].T
        W_x.append(keras_W_x[0 * hidden_size:][:hidden_size])
        W_x.append(keras_W_x[1 * hidden_size:][:hidden_size])
        W_x.append(keras_W_x[3 * hidden_size:][:hidden_size])
        W_x.append(keras_W_x[2 * hidden_size:][:hidden_size])

        keras_b = keras_layer.get_weights()[2]
        b.append(keras_b[0 * hidden_size:][:hidden_size])
        b.append(keras_b[1 * hidden_size:][:hidden_size])
        b.append(keras_b[3 * hidden_size:][:hidden_size])
        b.append(keras_b[2 * hidden_size:][:hidden_size])

    # Set activation type
    inner_activation_str = _get_recurrent_activation_name_from_keras(keras_layer.inner_activation)
    activation_str = _get_recurrent_activation_name_from_keras(keras_layer.activation)

    # Add to the network
    builder.add_unilstm(
        name = layer,
        W_h = W_h, W_x = W_x, b = b,
        hidden_size = hidden_size,
        input_size = input_size,
        input_names = input_names,
        output_names = output_names,
        inner_activation = inner_activation_str,
        cell_state_update_activation = activation_str,
        output_activation = activation_str,
        output_all = output_all,
        reverse_input = reverse_input)


def convert_simple_rnn(builder, layer, input_names, output_names, keras_layer):
    """Convert an SimpleRNN layer from keras to coreml.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    # Get input and output names    
    hidden_size = keras_layer.output_dim
    input_size = keras_layer.input_shape[-1]

    output_all = keras_layer.return_sequences
    reverse_input = keras_layer.go_backwards

    if keras_layer.consume_less not in ['cpu', 'gpu']:
        raise ValueError('Cannot convert Keras layer with consume_less = %s'
                         % keras_layer.consume_less)
    
    W_h = np.zeros((hidden_size, hidden_size))
    W_x = np.zeros((hidden_size, input_size))
    b = np.zeros((hidden_size,))

    if keras_layer.consume_less == 'cpu':
        W_h = keras_layer.get_weights()[1].T
        W_x = keras_layer.get_weights()[0].T
        b = keras_layer.get_weights()[2]
    else:
        W_h = keras_layer.get_weights()[1].T
        W_x = keras_layer.get_weights()[0].T
        b = keras_layer.get_weights()[2]

    # Set actication type
    activation_str = _get_recurrent_activation_name_from_keras(keras_layer.activation)

    # Add to the network
    builder.add_simple_rnn(
        name = layer,
        W_h = W_h, W_x = W_x, b = b,
        hidden_size = hidden_size,
        input_size = input_size,
        activation = activation_str,
        input_names = input_names,
        output_names = output_names,
        output_all=output_all,
        reverse_input=reverse_input)

def convert_gru(builder, layer, input_names, output_names, keras_layer):
    """Convert a GRU layer from keras to coreml.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    
    hidden_size = keras_layer.output_dim
    input_size = keras_layer.input_shape[-1]

    output_all = keras_layer.return_sequences
    reverse_input = keras_layer.go_backwards

    if keras_layer.consume_less not in ['cpu', 'gpu']:
        raise ValueError('Cannot convert Keras layer with consume_less = %s'
                         % keras_layer.consume_less)

    # Keras: Z R O
    # CoreML: Z R O    
    W_h, W_x, b = ([], [], [])
    if keras_layer.consume_less == 'cpu':
        W_x.append(keras_layer.get_weights()[0].T)
        W_x.append(keras_layer.get_weights()[3].T)
        W_x.append(keras_layer.get_weights()[6].T)

        W_h.append(keras_layer.get_weights()[1].T)
        W_h.append(keras_layer.get_weights()[4].T)
        W_h.append(keras_layer.get_weights()[7].T)

        b.append(keras_layer.get_weights()[2])
        b.append(keras_layer.get_weights()[5])
        b.append(keras_layer.get_weights()[8])
    else:
        print('consume less not implemented')

    # Set actication type
    inner_activation_str = _get_recurrent_activation_name_from_keras(keras_layer.inner_activation)
    activation_str = _get_recurrent_activation_name_from_keras(keras_layer.activation)

    # Add to the network
    builder.add_gru(
       name = layer,
       W_h = W_h, W_x = W_x, b = b,
       input_size = input_size,
       hidden_size = hidden_size,
       input_names = input_names,
       output_names = output_names,
       activation = activation_str,
       inner_activation = inner_activation_str,
       output_all=output_all,
       reverse_input = reverse_input)


def convert_bidirectional(builder, layer, input_names, output_names, keras_layer):
    """Convert a bidirectional layer from keras to coreml.
        Currently assumes the units are LSTMs.

    Parameters
    ----------
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
        
    input_size = keras_layer.input_shape[-1]

    lstm_layer = keras_layer.forward_layer
    if (type(lstm_layer) != keras.layers.recurrent.LSTM):
        raise TypeError('Bidirectional layers only supported with LSTM')
        
    if lstm_layer.go_backwards:
        raise TypeError(' \'go_backwards\' mode not supported with Bidirectional layers')    

    output_all = keras_layer.return_sequences

    hidden_size = lstm_layer.output_dim
    #output_size = lstm_layer.output_dim * 2

    if lstm_layer.consume_less not in ['cpu', 'gpu']:
        raise ValueError('Cannot convert Keras layer with consume_less = %s'
                         % keras_layer.consume_less)

    # Keras: I C F O; W_x, W_h, b
    # CoreML: I F O G; W_h and W_x are separated

    # Keras has all forward weights, followed by backward in the same order
    W_h, W_x, b = ([], [], [])
    if lstm_layer.consume_less == 'cpu':
        W_h.append(keras_layer.get_weights()[1].T)
        W_h.append(keras_layer.get_weights()[7].T)
        W_h.append(keras_layer.get_weights()[10].T)
        W_h.append(keras_layer.get_weights()[4].T)

        W_x.append(keras_layer.get_weights()[0].T)
        W_x.append(keras_layer.get_weights()[6].T)
        W_x.append(keras_layer.get_weights()[9].T)
        W_x.append(keras_layer.get_weights()[3].T)

        b.append(keras_layer.get_weights()[2])
        b.append(keras_layer.get_weights()[8])
        b.append(keras_layer.get_weights()[11])
        b.append(keras_layer.get_weights()[5])
    else:
        keras_W_h = keras_layer.get_weights()[1].T
        W_h.append(keras_W_h[0 * hidden_size:][:hidden_size])
        W_h.append(keras_W_h[1 * hidden_size:][:hidden_size])
        W_h.append(keras_W_h[3 * hidden_size:][:hidden_size])
        W_h.append(keras_W_h[2 * hidden_size:][:hidden_size])

        keras_W_x = keras_layer.get_weights()[0].T
        W_x.append(keras_W_x[0 * hidden_size:][:hidden_size])
        W_x.append(keras_W_x[1 * hidden_size:][:hidden_size])
        W_x.append(keras_W_x[3 * hidden_size:][:hidden_size])
        W_x.append(keras_W_x[2 * hidden_size:][:hidden_size])

        keras_b = keras_layer.get_weights()[2]
        b.append(keras_b[0 * hidden_size:][:hidden_size])
        b.append(keras_b[1 * hidden_size:][:hidden_size])
        b.append(keras_b[3 * hidden_size:][:hidden_size])
        b.append(keras_b[2 * hidden_size:][:hidden_size])

    W_h_back, W_x_back, b_back = ([],[],[])
    if keras_layer.backward_layer.consume_less == 'cpu':
        back_weights = keras_layer.backward_layer.get_weights()
        W_h_back.append(back_weights[1].T)
        W_h_back.append(back_weights[7].T)
        W_h_back.append(back_weights[10].T)
        W_h_back.append(back_weights[4].T)

        W_x_back.append(back_weights[0].T)
        W_x_back.append(back_weights[6].T)
        W_x_back.append(back_weights[9].T)
        W_x_back.append(back_weights[3].T)

        b_back.append(back_weights[2])
        b_back.append(back_weights[8])
        b_back.append(back_weights[11])
        b_back.append(back_weights[5])
    else:
        keras_W_h = keras_layer.backward_layer.get_weights()[1].T
        W_h_back.append(keras_W_h[0 * hidden_size:][:hidden_size])
        W_h_back.append(keras_W_h[1 * hidden_size:][:hidden_size])
        W_h_back.append(keras_W_h[3 * hidden_size:][:hidden_size])
        W_h_back.append(keras_W_h[2 * hidden_size:][:hidden_size])

        keras_W_x = keras_layer.backward_layer.get_weights()[0].T
        W_x_back.append(keras_W_x[0 * hidden_size:][:hidden_size])
        W_x_back.append(keras_W_x[1 * hidden_size:][:hidden_size])
        W_x_back.append(keras_W_x[3 * hidden_size:][:hidden_size])
        W_x_back.append(keras_W_x[2 * hidden_size:][:hidden_size])

        keras_b = keras_layer.backward_layer.get_weights()[2]
        b_back.append(keras_b[0 * hidden_size:][:hidden_size])
        b_back.append(keras_b[1 * hidden_size:][:hidden_size])
        b_back.append(keras_b[3 * hidden_size:][:hidden_size])
        b_back.append(keras_b[2 * hidden_size:][:hidden_size])

    # Set activation type
    inner_activation_str = _get_recurrent_activation_name_from_keras(lstm_layer.inner_activation)
    activation_str = _get_recurrent_activation_name_from_keras(lstm_layer.activation)


    # Add to the network
    builder.add_bidirlstm(
        name = layer,
        W_h = W_h, W_x = W_x, b = b, 
        W_h_back = W_h_back, W_x_back = W_x_back, b_back = b_back,
        hidden_size=hidden_size,
        input_size=input_size,
        input_names=input_names,
        output_names=output_names,
        inner_activation = inner_activation_str,
        cell_state_update_activation = activation_str,
        output_activation = activation_str,
        output_all = output_all)

def convert_batchnorm(builder, layer, input_names, output_names, keras_layer):
    """
    Parameters
    keras_layer: layer
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """

    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])

    # Currently CoreML supports only per-channel batch-norm
    if keras_layer.mode != 0:
        raise NotImplementedError(
            'Currently supports only per-feature normalization')
    
    axis = keras_layer.axis
    nb_channels = keras_layer.input_shape[axis]
    

    # Set parameters
    # Parameter arrangement in Keras: gamma, beta, mean, variance
    gamma = keras_layer.get_weights()[0]
    beta = keras_layer.get_weights()[1]
    mean = keras_layer.get_weights()[2]
    std = keras_layer.get_weights()[3]
    # compute adjusted parameters 
    variance = std * std
    f = 1.0 / np.sqrt(std + keras_layer.epsilon)
    gamma1 = gamma*f
    beta1 = beta - gamma*mean*f
    mean[:] = 0.0 #mean
    variance[:] = 1.0 - .00001 #stddev

    builder.add_batchnorm(
        name = layer,
        channels = nb_channels,
        gamma = gamma1,
        beta = beta1,
        mean = mean, 
        variance = variance,
        input_name = input_name,
        output_name = output_name)

def convert_flatten(builder, layer, input_names, output_names, keras_layer):
    """Convert a flatten layer from keras to coreml.

    Parameters
    keras_layer: layer
    ----------
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    input_name, output_name = (input_names[0], output_names[0])
    
    # blob_order == 0 if the input blob needs not be rearranged
    # blob_order == 1 if the input blob needs to be rearranged
    blob_order = 0
    
    # using keras_layer.input.shape have a "?" (Dimension[None] at the front), 
    # making a 3D tensor with unknown batch size 4D
    if len(keras_layer.input.shape) == 4:
        blob_order = 1
    
    builder.add_flatten(name=layer, mode=blob_order, input_name=input_name,
                        output_name=output_name)

def convert_softmax(builder, layer, input_names, output_names, keras_layer):
    """Convert a softmax layer from keras to coreml.

    Parameters
    keras_layer: layer
    ----------
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    input_name, output_name = (input_names[0], output_names[0])
    
    builder.add_softmax(name = layer, input_name = input_name,
            output_name = output_name)

def convert_permute(builder, layer, input_names, output_names, keras_layer):
    """Convert a softmax layer from keras to coreml.

    Parameters
    keras_layer: layer
    ----------
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    input_name, output_name = (input_names[0], output_names[0])
    
    keras_dims = keras_layer.dims
    # Keras permute layer index begins at 1
    if len(keras_dims) == 3: 
        # Keras input tensor interpret as (H,W,C)
        x = list(np.array(keras_dims))
        i1, i2, i3 = x.index(1), x.index(2), x.index(3)
        x[i1], x[i2], x[i3] = 2, 3, 1
        # add a sequence axis
        x = [0] + x
        dim = tuple(x)
    elif len(keras_dims) == 4:
        # Here we use Keras converter as a place holder for inserting
        # permutations - the values here are not valid Keras dim parameters
        # but parameters we need to use to convert to CoreML model
        dim = keras_dims
    else: 
        raise NotImplementedError('Supports only 3d permutation.')
    
    builder.add_permute(name = layer, dim=dim, input_name = input_name,
            output_name = output_name)

def convert_embedding(builder, layer, input_names, output_names, keras_layer):
    """Convert a dense layer from keras to coreml.

    Parameters
    keras_layer: layer
    ----------
        A keras layer object.

    builder: NeuralNetworkBuilder
        A neural network builder object.
    """
    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])

    # Get the weights from keras
    W = keras_layer.get_weights ()[0].T

    # assuming keras embedding layers don't have biases 
    builder.add_embedding(name = layer,
                          W = W,
                          b = None,
                          input_dim = keras_layer.input_dim,
                          output_channels = keras_layer.output_dim,
                          has_bias = False,
                          input_name = input_name,
                          output_name = output_name)

def convert_repeat_vector(builder, layer, input_names, output_names, keras_layer):
    # Keras RepeatVector only deals with 1D input
    # Get input and output names
    input_name, output_name = (input_names[0], output_names[0])

    builder.add_sequence_repeat(name = layer,
            nrep = keras_layer.n, 
            input_name = input_name,
            output_name = output_name)

def default_skip(builder, layer, input_names, output_names, keras_layer):
    """ Layers that can be skipped (because they are train time only. """
    return
