from six import string_types as _string_types

from ...models.neural_network import NeuralNetworkBuilder as _NeuralNetworkBuilder
from ...proto import FeatureTypes_pb2 as _FeatureTypes_pb2
from collections import OrderedDict as _OrderedDict
from ...models import datatypes
from ...models import MLModel as _MLModel
from ...models.utils import save_spec as _save_spec

from ..._deps import HAS_KERAS2_TF as _HAS_KERAS2_TF

if _HAS_KERAS2_TF:
    import keras as _keras
    from . import _layers2
    from . import _topology2
    _KERAS_LAYER_REGISTRY  = {
        _keras.layers.core.Dense: _layers2.convert_dense,
        _keras.layers.core.Activation: _layers2.convert_activation,
        _keras.layers.advanced_activations.LeakyReLU: _layers2.convert_activation,
        _keras.layers.advanced_activations.PReLU: _layers2.convert_activation,
        _keras.layers.advanced_activations.ELU: _layers2.convert_activation,
        _keras.layers.advanced_activations.ThresholdedReLU: _layers2.convert_activation,

        _keras.layers.convolutional.Conv2D: _layers2.convert_convolution,
        _keras.layers.convolutional.Conv2DTranspose: _layers2.convert_convolution,
        _keras.layers.convolutional.SeparableConv2D: _layers2.convert_separable_convolution,
        _keras.layers.pooling.AveragePooling2D: _layers2.convert_pooling,
        _keras.layers.pooling.MaxPooling2D: _layers2.convert_pooling,
        _keras.layers.pooling.GlobalAveragePooling2D: _layers2.convert_pooling,
        _keras.layers.pooling.GlobalMaxPooling2D: _layers2.convert_pooling,
        _keras.layers.convolutional.ZeroPadding2D: _layers2.convert_padding,
        _keras.layers.convolutional.Cropping2D: _layers2.convert_cropping,
        _keras.layers.convolutional.UpSampling2D: _layers2.convert_upsample,

        _keras.layers.convolutional.Conv1D: _layers2.convert_convolution1d,
        _keras.layers.pooling.AveragePooling1D: _layers2.convert_pooling,
        _keras.layers.pooling.MaxPooling1D: _layers2.convert_pooling,
        _keras.layers.pooling.GlobalAveragePooling1D: _layers2.convert_pooling,
        _keras.layers.pooling.GlobalMaxPooling1D: _layers2.convert_pooling,
        _keras.layers.convolutional.ZeroPadding1D: _layers2.convert_padding,
        _keras.layers.convolutional.Cropping1D: _layers2.convert_cropping,
        _keras.layers.convolutional.UpSampling1D: _layers2.convert_upsample,

        _keras.layers.recurrent.LSTM: _layers2.convert_lstm,
        _keras.layers.recurrent.SimpleRNN: _layers2.convert_simple_rnn,
        _keras.layers.recurrent.GRU: _layers2.convert_gru,
        _keras.layers.wrappers.Bidirectional: _layers2.convert_bidirectional,

        _keras.layers.normalization.BatchNormalization: _layers2.convert_batchnorm,

        _keras.layers.Add: _layers2.convert_merge,
        _keras.layers.Multiply: _layers2.convert_merge,
        _keras.layers.Average: _layers2.convert_merge,
        _keras.layers.Maximum: _layers2.convert_merge,
        _keras.layers.Concatenate: _layers2.convert_merge,
        _keras.layers.Dot: _layers2.convert_merge,

        _keras.layers.core.Flatten: _layers2.convert_flatten,
        _keras.layers.core.Permute:_layers2.convert_permute,
        _keras.layers.core.Reshape:_layers2.convert_reshape,
        _keras.layers.embeddings.Embedding:_layers2.convert_embedding,
        _keras.layers.core.RepeatVector:_layers2.convert_repeat_vector,

        _keras.layers.core.Dropout:_layers2.default_skip,
        _keras.layers.core.SpatialDropout2D:_layers2.default_skip,
        _keras.layers.core.SpatialDropout1D:_layers2.default_skip,
        _keras.layers.wrappers.TimeDistributed:_layers2.default_skip,
    }
    from distutils.version import StrictVersion as _StrictVersion
    ## 2.2 Version check
    if _keras.__version__ >= _StrictVersion('2.2.0'):
         _KERAS_LAYER_REGISTRY[_keras.layers.DepthwiseConv2D] = _layers2.convert_convolution
         _KERAS_LAYER_REGISTRY[_keras.engine.input_layer.InputLayer] = _layers2.default_skip
         if _keras.__version__ >= _StrictVersion('2.2.1'):
             _KERAS_LAYER_REGISTRY[_keras.layers.advanced_activations.ReLU] = _layers2.convert_advanced_relu
    else:
         _KERAS_LAYER_REGISTRY[_keras.applications.mobilenet.DepthwiseConv2D] = _layers2.convert_convolution
         _KERAS_LAYER_REGISTRY[_keras.engine.topology.InputLayer] = _layers2.default_skip
    # end if _HAS_KERAS2_TF


def _is_merge_layer(layer):
    if _HAS_KERAS2_TF:
        for lt in _topology2._KERAS_MERGE_LAYERS:
            if isinstance(layer, lt):
                return True
    return False

def _is_activation_layer(layer):
    return (isinstance(layer, _keras.layers.core.Activation) or \
            isinstance(layer, _keras.layers.advanced_activations.LeakyReLU) or \
            isinstance(layer, _keras.layers.advanced_activations.PReLU) or \
            isinstance(layer, _keras.layers.advanced_activations.ELU) or \
            isinstance(layer, _keras.layers.advanced_activations.ThresholdedReLU))

def _check_unsupported_layers(model, add_custom_layers = False):
    # When add_custom_layers = True, we just convert all layers not present in
    # registry as custom layer placeholders
    if add_custom_layers:
        return
    for i, layer in enumerate(model.layers):
        if isinstance(layer, _keras.models.Sequential) or isinstance(layer,
                _keras.models.Model):
            _check_unsupported_layers(layer)
        else:
            if type(layer) not in _KERAS_LAYER_REGISTRY:
                raise ValueError("Keras layer '%s' not supported. " % str(type(layer)))
            if isinstance(layer, _keras.layers.wrappers.TimeDistributed):
                if type(layer.layer) not in _KERAS_LAYER_REGISTRY:
                     raise ValueError(
                         "Keras layer '%s' not supported. " % str(type(layer.layer)))
            if isinstance(layer, _keras.layers.wrappers.Bidirectional):
                if not isinstance(layer.layer,  _keras.layers.recurrent.LSTM):
                    raise ValueError(
                        "Keras bi-directional wrapper conversion supports only LSTM layer at this time. ")

def _get_layer_converter_fn(layer, add_custom_layers = False):
    """Get the right converter function for Keras
    """
    layer_type = type(layer)
    if layer_type in _KERAS_LAYER_REGISTRY:
        convert_func = _KERAS_LAYER_REGISTRY[layer_type]
        if convert_func is _layers2.convert_activation:
            act_name = _layers2._get_activation_name_from_keras_layer(layer)
            if act_name == 'CUSTOM':
                return None
        return convert_func
    elif add_custom_layers:
        return None
    else:
        raise TypeError("Keras layer of type %s is not supported." % type(layer))


def _load_keras_model(model_network_path, model_weight_path, custom_objects=None):
    """Load a keras model from disk

    Parameters
    ----------
    model_network_path: str
        Path where the model network path is (json file)

    model_weight_path: str
        Path where the model network weights are (hd5 file)

    custom_objects:
        A dictionary of layers or other custom classes
        or functions used by the model

    Returns
    -------
    model: A keras model
    """
    from keras.models import model_from_json
    import json

    # Load the model network
    json_file = open(model_network_path, 'r')
    loaded_model_json = json_file.read()
    json_file.close()

    if not custom_objects:
        custom_objects = {}

    # Load the model weights
    loaded_model = model_from_json(loaded_model_json, custom_objects=custom_objects)
    loaded_model.load_weights(model_weight_path)

    return loaded_model


def _convert(model,
            input_names = None,
            output_names = None,
            image_input_names = None,
            input_name_shape_dict = {},
            is_bgr = False,
            red_bias = 0.0,
            green_bias = 0.0,
            blue_bias = 0.0,
            gray_bias = 0.0,
            image_scale = 1.0,
            class_labels = None,
            predicted_feature_name = None,
            predicted_probabilities_output = '',
            add_custom_layers = False,
            custom_conversion_functions = None,
            custom_objects = None):

    # Check Keras format
    if _keras.backend.image_data_format() == 'channels_first':
        print("Keras image data format 'channels_first' detected. Currently only 'channels_last' is supported. "
            "Changing to 'channels_last', but your model may not be converted "
            "converted properly.")
        _keras.backend.set_image_data_format('channels_last')

    # Check custom conversion functions / custom objects
    add_custom_layers = custom_conversion_functions is not None

    if isinstance(model, _string_types):
        model = _keras.models.load_model(model, custom_objects = custom_objects)
    elif isinstance(model, tuple):
        model = _load_keras_model(model[0], model[1])

    # Check valid versions
    _check_unsupported_layers(model, add_custom_layers)

    # Build network graph to represent Keras model
    graph = _topology2.NetGraph(model)
    graph.build()

    # The graph should be finalized before executing this
    graph.generate_blob_names()
    graph.add_recurrent_optionals()

    inputs = graph.get_input_layers()
    outputs = graph.get_output_layers()

    # check input / output names validity
    if input_names is not None:
        if isinstance(input_names, _string_types):
            input_names = [input_names]
    else:
        input_names = ['input' + str(i+1) for i in range(len(inputs))]
    if output_names is not None:
        if isinstance(output_names, _string_types):
            output_names = [output_names]
    else:
        output_names = ['output' + str(i+1) for i in range(len(outputs))]

    if image_input_names is not None and isinstance(image_input_names, _string_types):
        image_input_names = [image_input_names]

    graph.reset_model_input_names(input_names)
    graph.reset_model_output_names(output_names)

    # Keras -> Core ML input dimension dictionary
    # (None, None) -> [1, 1, 1, 1, 1]
    # (None, D) -> [D] or [D, 1, 1, 1, 1]
    # (None, Seq, D) -> [Seq, 1, D, 1, 1]
    # (None, H, W, C) -> [C, H, W]
    # (D) -> [D]
    # (Seq, D) -> [Seq, 1, D, 1, 1]
    # (Batch, Sequence, D) -> [D]
    # (Batch, Seq, H, W, C) -> (C,H,W)

    # Retrieve input shapes from model
    if type(model.input_shape) is list:
        input_dims = [filter(None, x) for x in model.input_shape]
        unfiltered_shapes = model.input_shape
    else:
        input_dims = [filter(None, model.input_shape)]
        unfiltered_shapes = [model.input_shape]

    for idx, dim in enumerate(input_dims):
        if input_names[idx] in input_name_shape_dict:
            unfiltered_shape = input_name_shape_dict[input_names[idx]]
            dim = list(filter(None,unfiltered_shape))
        else:
            unfiltered_shape = unfiltered_shapes[idx]
            dim = list(input_dims[idx])

        if len(unfiltered_shape) == 1:
            if len(dim) == 1:
                input_dims[idx] = dim  # dim is just a number
            else:
                errMsg =  "Invalid input shape for {}.\n".format(input_names[idx])
                errMsg += "Please provide a finite channel value (D) using input_name_shape_dict arg "
                errMsg += "with key = '{}' and value = [D]".format(input_names[idx])
                raise ValueError(errMsg)

        elif len(unfiltered_shape) == 2:
            if len(dim) == 2:# [Seq, D]
                input_dims[idx] = (dim[1],)
            elif len(dim) == 1:
                s = graph.get_successors(inputs[idx])[0]
                if isinstance(graph.get_keras_layer(s), _keras.layers.embeddings.Embedding):
                    # Embedding layer's special input (None, D) where D is actually sequence length
                    input_dims[idx] = (1,)
                else:
                    input_dims[idx] = dim  # dim is just a number
            else: # Used to be [None, None] before filtering; indicating unknown sequence length
                input_dims[idx] = tuple([1])

        elif len(unfiltered_shape) == 3:
            if len(dim) == 3:# keras provided fixed batch and sequence length, so the input was (batch, sequence, channel)
                input_dims[idx] = (dim[2],)
            elif len(dim) == 2:# [None, Seq, D]
                input_dims[idx] = (dim[1],)
            elif len(dim) == 1:
                input_dims[idx] = dim  # dim is just a number
            else:
                errMsg =  "Invalid input shape for {}.\n".format(input_names[idx])
                errMsg += "Please provide a finite channel value (D) using "
                errMsg += "input_name_shape_dict arg with key = '{}' and value = [None,None,D]".format(input_names[idx])
                raise ValueError(errMsg)

        elif len(unfiltered_shape) == 4:
            if len(dim) == 3:# keras uses the reverse notation from CoreML
                input_dims[idx] = (dim[2], dim[0], dim[1])
            else:
                errMsg =  "Invalid input shape for {}.\n".format(input_names[idx])
                errMsg += "Please provide a finite height (H), width (W) & channel value (C) "
                errMsg += "using input_name_shape_dict arg with key = '{}' and value = [None,H,W,C]\n".format(input_names[idx])
                errMsg += "Converted .mlmodel can be modified to have flexible input shape using coremltools.models.neural_network.flexible_shape_utils"
                raise ValueError(errMsg)

        elif len(unfiltered_shape) == 5:
            if len(dim) == 4:# keras uses the reverse notation from CoreML
                input_dims[idx] = (dim[-1], dim[-3], dim[-2])
            else:
                errMsg = "Invalid input shape for input: {}, shape:{}.\n".format(input_names[idx], str(unfiltered_shape))
                raise ValueError(errMsg)
        else:
            raise ValueError("Input '%s' has input shape of length %d" % (input_names[idx], len(dim)))


    # Retrieve output shapes from model
    if type(model.output_shape) is list:
        output_dims = [filter(None, x) for x in model.output_shape]
    else:
        output_dims = [filter(None, model.output_shape[1:])]

    for idx, dim in enumerate(output_dims):
        dim = list(dim)
        if len(dim) == 1:
            output_dims[idx] = dim
        elif len(dim) == 2:  # [Seq, D]
            output_dims[idx] = (dim[1],)
        elif len(dim) == 3:
            output_dims[idx] = (dim[2], dim[0], dim[1])

    input_types = [datatypes.Array(*dim) for dim in input_dims]
    output_types = [datatypes.Array(*dim) for dim in output_dims]

    # Some of the feature handling is sensitive about string vs. unicode
    input_names = map(str, input_names)
    output_names = map(str, output_names)
    is_classifier = class_labels is not None
    if is_classifier:
        mode = 'classifier'
    else:
        mode = None

    # assuming these match
    input_features = list(zip(input_names, input_types))
    output_features = list(zip(output_names, output_types))

    builder = _NeuralNetworkBuilder(input_features, output_features, mode = mode)

    for iter, layer in enumerate(graph.layer_list):
        keras_layer = graph.keras_layer_map[layer]
        print("%d : %s, %s" % (iter, layer, keras_layer))
        if isinstance(keras_layer, _keras.layers.wrappers.TimeDistributed):
            keras_layer = keras_layer.layer
        converter_func = _get_layer_converter_fn(keras_layer, add_custom_layers)
        input_names, output_names = graph.get_layer_blobs(layer)
        # this may be none if we're using custom layers
        if converter_func:
            converter_func(builder, layer, input_names, output_names, keras_layer)
        else:
            if _is_activation_layer(keras_layer):
                import six
                if six.PY2:
                    layer_name = keras_layer.activation.func_name
                else:
                    layer_name = keras_layer.activation.__name__
            else:
                layer_name = type(keras_layer).__name__
            if layer_name in custom_conversion_functions:
                custom_spec = custom_conversion_functions[layer_name](keras_layer)
            else:
                custom_spec = None

            builder.add_custom(layer, input_names, output_names, custom_spec)

    # Since we aren't mangling anything the user gave us, we only need to update
    # the model interface here
    builder.add_optionals(graph.optional_inputs, graph.optional_outputs)

    # Add classifier classes (if applicable)
    if is_classifier:
        classes_in = class_labels
        if isinstance(classes_in, _string_types):
            import os
            if not os.path.isfile(classes_in):
                raise ValueError("Path to class labels (%s) does not exist." % classes_in)
            with open(classes_in, 'r') as f:
                classes = f.read()
            classes = classes.splitlines()
        elif type(classes_in) is list: # list[int or str]
            classes = classes_in
        else:
            raise ValueError('Class labels must be a list of integers / strings, or a file path')

        if predicted_feature_name is not None:
            builder.set_class_labels(classes, predicted_feature_name = predicted_feature_name,
                                     prediction_blob = predicted_probabilities_output)
        else:
            builder.set_class_labels(classes)

    # Set pre-processing paramsters
    builder.set_pre_processing_parameters(image_input_names = image_input_names,
                                          is_bgr = is_bgr,
                                          red_bias = red_bias,
                                          green_bias = green_bias,
                                          blue_bias = blue_bias,
                                          gray_bias = gray_bias,
                                          image_scale = image_scale)

    # Return the protobuf spec
    spec = builder.spec
    return spec
