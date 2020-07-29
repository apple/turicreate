# Copyright (c) 2017-2019, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from six import string_types as _string_types

from ...models.neural_network import NeuralNetworkBuilder as _NeuralNetworkBuilder
from ...proto import FeatureTypes_pb2 as _FeatureTypes_pb2
from collections import OrderedDict as _OrderedDict
from ...models import datatypes, _METADATA_VERSION, _METADATA_SOURCE
from ...models import MLModel as _MLModel
from ...models import (
    _MLMODEL_FULL_PRECISION,
    _MLMODEL_HALF_PRECISION,
    _VALID_MLMODEL_PRECISION_TYPES,
)
from ...models.utils import _convert_neural_network_spec_weights_to_fp16

from ..._deps import _HAS_KERAS_TF
from ..._deps import _HAS_KERAS2_TF
from coremltools import __version__ as ct_version

if _HAS_KERAS_TF:
    import keras as _keras
    from . import _layers
    from . import _topology

    _KERAS_LAYER_REGISTRY = {
        _keras.layers.core.Dense: _layers.convert_dense,
        _keras.layers.core.Activation: _layers.convert_activation,
        _keras.layers.advanced_activations.LeakyReLU: _layers.convert_activation,
        _keras.layers.advanced_activations.PReLU: _layers.convert_activation,
        _keras.layers.advanced_activations.ELU: _layers.convert_activation,
        _keras.layers.advanced_activations.ParametricSoftplus: _layers.convert_activation,
        _keras.layers.advanced_activations.ThresholdedReLU: _layers.convert_activation,
        _keras.activations.softmax: _layers.convert_activation,
        _keras.layers.convolutional.Convolution2D: _layers.convert_convolution,
        _keras.layers.convolutional.Deconvolution2D: _layers.convert_convolution,
        _keras.layers.convolutional.AtrousConvolution2D: _layers.convert_convolution,
        _keras.layers.convolutional.AveragePooling2D: _layers.convert_pooling,
        _keras.layers.convolutional.MaxPooling2D: _layers.convert_pooling,
        _keras.layers.pooling.GlobalAveragePooling2D: _layers.convert_pooling,
        _keras.layers.pooling.GlobalMaxPooling2D: _layers.convert_pooling,
        _keras.layers.convolutional.ZeroPadding2D: _layers.convert_padding,
        _keras.layers.convolutional.Cropping2D: _layers.convert_cropping,
        _keras.layers.convolutional.UpSampling2D: _layers.convert_upsample,
        _keras.layers.convolutional.Convolution1D: _layers.convert_convolution1d,
        _keras.layers.convolutional.AtrousConvolution1D: _layers.convert_convolution1d,
        _keras.layers.convolutional.AveragePooling1D: _layers.convert_pooling,
        _keras.layers.convolutional.MaxPooling1D: _layers.convert_pooling,
        _keras.layers.pooling.GlobalAveragePooling1D: _layers.convert_pooling,
        _keras.layers.pooling.GlobalMaxPooling1D: _layers.convert_pooling,
        _keras.layers.convolutional.ZeroPadding1D: _layers.convert_padding,
        _keras.layers.convolutional.Cropping1D: _layers.convert_cropping,
        _keras.layers.convolutional.UpSampling1D: _layers.convert_upsample,
        _keras.layers.recurrent.LSTM: _layers.convert_lstm,
        _keras.layers.recurrent.SimpleRNN: _layers.convert_simple_rnn,
        _keras.layers.recurrent.GRU: _layers.convert_gru,
        _keras.layers.wrappers.Bidirectional: _layers.convert_bidirectional,
        _keras.layers.normalization.BatchNormalization: _layers.convert_batchnorm,
        _keras.engine.topology.Merge: _layers.convert_merge,
        _keras.layers.core.Flatten: _layers.convert_flatten,
        _keras.layers.core.Permute: _layers.convert_permute,
        _keras.layers.core.Reshape: _layers.convert_reshape,
        _keras.layers.embeddings.Embedding: _layers.convert_embedding,
        _keras.layers.core.RepeatVector: _layers.convert_repeat_vector,
        ## All the layers that can be skipped (merged with conv)
        _keras.engine.topology.InputLayer: _layers.default_skip,
        _keras.layers.core.Dropout: _layers.default_skip,
        _keras.layers.wrappers.TimeDistributed: _layers.default_skip,
    }

    _KERAS_SKIP_LAYERS = [
        _keras.layers.core.Dropout,
    ]


def _check_unsupported_layers(model):
    for i, layer in enumerate(model.layers):
        if isinstance(layer, _keras.models.Sequential) or isinstance(
            layer, _keras.models.Model
        ):
            _check_unsupported_layers(layer)
        else:
            if type(layer) not in _KERAS_LAYER_REGISTRY:
                raise ValueError("Keras layer '%s' not supported. " % str(type(layer)))
            if isinstance(layer, _keras.engine.topology.Merge):
                if layer.layers is None:
                    continue
                for merge_layer in layer.layers:
                    if isinstance(merge_layer, _keras.models.Sequential) or isinstance(
                        merge_layer, _keras.models.Model
                    ):
                        _check_unsupported_layers(merge_layer)
            if isinstance(layer, _keras.layers.wrappers.TimeDistributed):
                if type(layer.layer) not in _KERAS_LAYER_REGISTRY:
                    raise ValueError(
                        "Keras layer '%s' not supported. " % str(type(layer.layer))
                    )
            if isinstance(layer, _keras.layers.wrappers.Bidirectional):
                if not isinstance(layer.layer, _keras.layers.recurrent.LSTM):
                    raise ValueError(
                        "Keras bi-directional wrapper conversion supports only "
                        "LSTM layer at this time. "
                    )


def _get_layer_converter_fn(layer):
    """Get the right converter function for Keras
    """
    layer_type = type(layer)
    if layer_type in _KERAS_LAYER_REGISTRY:
        return _KERAS_LAYER_REGISTRY[layer_type]
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
    json_file = open(model_network_path, "r")
    json_string = json_file.read()
    json_file.close()
    loaded_model_json = json.loads(json_string)

    if not custom_objects:
        custom_objects = {}

    # Load the model weights
    loaded_model = model_from_json(loaded_model_json, custom_objects=custom_objects)
    loaded_model.load_weights(model_weight_path)

    return loaded_model


def _convert(
    model,
    input_names=None,
    output_names=None,
    image_input_names=None,
    is_bgr=False,
    red_bias=0.0,
    green_bias=0.0,
    blue_bias=0.0,
    gray_bias=0.0,
    image_scale=1.0,
    class_labels=None,
    predicted_feature_name=None,
    predicted_probabilities_output="",
    custom_objects=None,
    respect_trainable=False,
):
    if not (_HAS_KERAS_TF):
        raise RuntimeError(
            "keras not found or unsupported version or backend "
            "found. keras conversion API is disabled."
        )
    if isinstance(model, _string_types):
        model = _keras.models.load_model(model, custom_objects=custom_objects)
    elif isinstance(model, tuple):
        model = _load_keras_model(model[0], model[1], custom_objects=custom_objects)

    # Check valid versions
    _check_unsupported_layers(model)

    # Build network graph to represent Keras model
    graph = _topology.NetGraph(model)
    graph.build()
    graph.remove_skip_layers(_KERAS_SKIP_LAYERS)
    graph.insert_1d_permute_layers()
    graph.insert_permute_for_spatial_bn()
    graph.defuse_activation()
    graph.remove_internal_input_layers()
    graph.make_output_layers()

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
        input_names = ["input" + str(i + 1) for i in range(len(inputs))]
    if output_names is not None:
        if isinstance(output_names, _string_types):
            output_names = [output_names]
    else:
        output_names = ["output" + str(i + 1) for i in range(len(outputs))]

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
    # (Seq, D) -> [Seq, 1, 1, D, 1]
    # (Batch, Sequence, D) -> [D]

    # Retrieve input shapes from model
    if type(model.input_shape) is list:
        input_dims = [list(filter(None, x)) for x in model.input_shape]
        unfiltered_shapes = model.input_shape
    else:
        input_dims = [list(filter(None, model.input_shape))]
        unfiltered_shapes = [model.input_shape]

    for idx, dim in enumerate(input_dims):
        unfiltered_shape = unfiltered_shapes[idx]
        if len(dim) == 0:
            # Used to be [None, None] before filtering; indicating unknown
            # sequence length
            input_dims[idx] = tuple([1])
        elif len(dim) == 1:
            s = graph.get_successors(inputs[idx])[0]
            if isinstance(graph.get_keras_layer(s), _keras.layers.embeddings.Embedding):
                # Embedding layer's special input (None, D) where D is actually
                # sequence length
                input_dims[idx] = (1,)
            else:
                input_dims[idx] = dim  # dim is just a number
        elif len(dim) == 2:  # [Seq, D]
            input_dims[idx] = (dim[1],)
        elif len(dim) == 3:  # H,W,C
            if len(unfiltered_shape) > 3:
                # keras uses the reverse notation from us
                input_dims[idx] = (dim[2], dim[0], dim[1])
            else:  # keras provided fixed batch and sequence length, so the input
                # was (batch, sequence, channel)
                input_dims[idx] = (dim[2],)
        else:
            raise ValueError(
                "Input" + input_names[idx] + "has input shape of length" + str(len(dim))
            )

    # Retrieve output shapes from model
    if type(model.output_shape) is list:
        output_dims = [list(filter(None, x)) for x in model.output_shape]
    else:
        output_dims = [list(filter(None, model.output_shape[1:]))]

    for idx, dim in enumerate(output_dims):
        if len(dim) == 1:
            output_dims[idx] = dim
        elif len(dim) == 2:  # [Seq, D]
            output_dims[idx] = (dim[1],)
        elif len(dim) == 3:
            output_dims[idx] = (dim[2], dim[1], dim[0])

    input_types = [datatypes.Array(*dim) for dim in input_dims]
    output_types = [datatypes.Array(*dim) for dim in output_dims]

    # Some of the feature handling is sensitive about string vs. unicode
    input_names = map(str, input_names)
    output_names = map(str, output_names)
    is_classifier = class_labels is not None
    if is_classifier:
        mode = "classifier"
    else:
        mode = None

    # assuming these match
    input_features = list(zip(input_names, input_types))
    output_features = list(zip(output_names, output_types))

    builder = _NeuralNetworkBuilder(input_features, output_features, mode=mode)

    for iter, layer in enumerate(graph.layer_list):
        keras_layer = graph.keras_layer_map[layer]
        print("%d : %s, %s" % (iter, layer, keras_layer))
        if isinstance(keras_layer, _keras.layers.wrappers.TimeDistributed):
            keras_layer = keras_layer.layer
        converter_func = _get_layer_converter_fn(keras_layer)
        input_names, output_names = graph.get_layer_blobs(layer)
        converter_func(builder, layer, input_names, output_names, keras_layer)

    # Set the right inputs and outputs on the model description (interface)
    builder.set_input(input_names, input_dims)
    builder.set_output(output_names, output_dims)

    # Since we aren't mangling anything the user gave us, we only need to update
    # the model interface here
    builder.add_optionals(graph.optional_inputs, graph.optional_outputs)

    # Add classifier classes (if applicable)
    if is_classifier:
        classes_in = class_labels
        if isinstance(classes_in, _string_types):
            import os

            if not os.path.isfile(classes_in):
                raise ValueError(
                    "Path to class labels (%s) does not exist." % classes_in
                )
            with open(classes_in, "r") as f:
                classes = f.read()
            classes = classes.splitlines()
        elif type(classes_in) is list:  # list[int or str]
            classes = classes_in
        else:
            raise ValueError(
                "Class labels must be a list of integers / strings, or a file path"
            )

        if predicted_feature_name is not None:
            builder.set_class_labels(
                classes,
                predicted_feature_name=predicted_feature_name,
                prediction_blob=predicted_probabilities_output,
            )
        else:
            builder.set_class_labels(classes)

    # Set pre-processing paramsters
    builder.set_pre_processing_parameters(
        image_input_names=image_input_names,
        is_bgr=is_bgr,
        red_bias=red_bias,
        green_bias=green_bias,
        blue_bias=blue_bias,
        gray_bias=gray_bias,
        image_scale=image_scale,
    )

    # Return the protobuf spec
    spec = builder.spec
    return spec


def _convert_to_spec(
    model,
    input_names=None,
    output_names=None,
    image_input_names=None,
    input_name_shape_dict={},
    is_bgr=False,
    red_bias=0.0,
    green_bias=0.0,
    blue_bias=0.0,
    gray_bias=0.0,
    image_scale=1.0,
    class_labels=None,
    predicted_feature_name=None,
    model_precision=_MLMODEL_FULL_PRECISION,
    predicted_probabilities_output="",
    add_custom_layers=False,
    custom_conversion_functions=None,
    custom_objects=None,
    input_shapes=None,
    output_shapes=None,
    respect_trainable=False,
    use_float_arraytype=False,
):
    """
    Convert a Keras model to Core ML protobuf specification (.mlmodel).

    Parameters
    ----------
    model: Keras model object | str | (str, str)
        A trained Keras neural network model which can be one of the following:

        - a Keras model object
        - a string with the path to a Keras model file (h5)
        - a tuple of strings, where the first is the path to a Keras model

          architecture (.json file), the second is the path to its weights
          stored in h5 file.

    input_names: [str] | str
        Optional name(s) that can be given to the inputs of the Keras model.
        These names will be used in the interface of the Core ML models to refer
        to the inputs of the Keras model. If not provided, the Keras inputs
        are named to [input1, input2, ..., inputN] in the Core ML model.  When
        multiple inputs are present, the input feature names are in the same
        order as the Keras inputs.

    output_names: [str] | str
        Optional name(s) that can be given to the outputs of the Keras model.
        These names will be used in the interface of the Core ML models to refer
        to the outputs of the Keras model. If not provided, the Keras outputs
        are named to [output1, output2, ..., outputN] in the Core ML model.
        When multiple outputs are present, output feature names are in the same
        order as the Keras inputs.

    image_input_names: [str] | str
        Input names to the Keras model (a subset of the input_names
        parameter) that can be treated as images by Core ML. All other inputs
        are treated as MultiArrays (N-D Arrays).

    input_name_shape_dict: {str: [int]}
        Optional Dictionary of input tensor names and their corresponding shapes expressed
        as a list of ints

    is_bgr: bool | dict()
        Flag indicating the channel order the model internally uses to represent
        color images. Set to True if the internal channel order is BGR,
        otherwise it will be assumed RGB. This flag is applicable only if
        image_input_names is specified. To specify a different value for each
        image input, provide a dictionary with input names as keys.
        Note that this flag is about the models internal channel order.
        An input image can be passed to the model in any color pixel layout
        containing red, green and blue values (e.g. 32BGRA or 32ARGB). This flag
        determines how those pixel values get mapped to the internal multiarray
        representation.

    red_bias: float | dict()
        Bias value to be added to the red channel of the input image.
        Defaults to 0.0
        Applicable only if image_input_names is specified.
        To specify different values for each image input provide a dictionary with input names as keys.

    blue_bias: float | dict()
        Bias value to be added to the blue channel of the input image.
        Defaults to 0.0
        Applicable only if image_input_names is specified.
        To specify different values for each image input provide a dictionary with input names as keys.

    green_bias: float | dict()
        Bias value to be added to the green channel of the input image.
        Defaults to 0.0
        Applicable only if image_input_names is specified.
        To specify different values for each image input provide a dictionary with input names as keys.

    gray_bias: float | dict()
        Bias value to be added to the input image (in grayscale). Defaults
        to 0.0
        Applicable only if image_input_names is specified.
        To specify different values for each image input provide a dictionary with input names as keys.

    image_scale: float | dict()
        Value by which input images will be scaled before bias is added and
        Core ML model makes a prediction. Defaults to 1.0.
        Applicable only if image_input_names is specified.
        To specify different values for each image input provide a dictionary with input names as keys.

    class_labels: list[int or str] | str
        Class labels (applies to classifiers only) that map the index of the
        output of a neural network to labels in a classifier.

        If the provided class_labels is a string, it is assumed to be a
        filepath where classes are parsed as a list of newline separated
        strings.

    predicted_feature_name: str
        Name of the output feature for the class labels exposed in the Core ML
        model (applies to classifiers only). Defaults to 'classLabel'

    model_precision: str
        Precision at which model will be saved. Currently full precision (float) and half precision
        (float16) models are supported. Defaults to '_MLMODEL_FULL_PRECISION' (full precision).

    predicted_probabilities_output: str
        Name of the neural network output to be interpreted as the predicted
        probabilities of the resulting classes. Typically the output of a
        softmax function. Defaults to the first output blob.

    add_custom_layers: bool
        If True, then unknown Keras layer types will be added to the model as
        'custom' layers, which must then be filled in as postprocessing.

    custom_conversion_functions: {'str': (Layer -> CustomLayerParams)}
        A dictionary with keys corresponding to names of custom layers and values
        as functions taking a Keras custom layer and returning a parameter dictionary
        and list of weights.

    custom_objects: {'str': (function)}
        Dictionary that includes a key, value pair of {'<function name>': <function>}
        for custom objects such as custom loss in the Keras model.
        Provide a string of the name of the custom function as a key.
        Provide a function as a value.

    respect_trainable: bool
        If True, then Keras layers that are marked 'trainable' will
        automatically be marked updatable in the Core ML model.

    use_float_arraytype: bool
        If true, the datatype of input/output multiarrays is set to Float32 instead
        of double.

    Returns
    -------
    model: MLModel
        Model in Core ML format.

    Examples
    --------
    .. sourcecode:: python

        # Make a Keras model
        >>> model = Sequential()
        >>> model.add(Dense(num_channels, input_dim = input_dim))

        # Convert it with default input and output names
        >>> import coremltools
        >>> coreml_model = coremltools.converters.keras.convert(model)

        # Saving the Core ML model to a file.
        >>> coreml_model.save('my_model.mlmodel')

    Converting a model with a single image input.

    .. sourcecode:: python

        >>> coreml_model = coremltools.converters.keras.convert(model, input_names =
        ... 'image', image_input_names = 'image')

    Core ML also lets you add class labels to models to expose them as
    classifiers.

    .. sourcecode:: python

        >>> coreml_model = coremltools.converters.keras.convert(model, input_names = 'image',
        ... image_input_names = 'image', class_labels = ['cat', 'dog', 'rat'])

    Class labels for classifiers can also come from a file on disk.

    .. sourcecode:: python

        >>> coreml_model = coremltools.converters.keras.convert(model, input_names =
        ... 'image', image_input_names = 'image', class_labels = 'labels.txt')

    Provide customized input and output names to the Keras inputs and outputs
    while exposing them to Core ML.

    .. sourcecode:: python

        >>> coreml_model = coremltools.converters.keras.convert(model, input_names =
        ...   ['my_input_1', 'my_input_2'], output_names = ['my_output'])

    """
    if model_precision not in _VALID_MLMODEL_PRECISION_TYPES:
        raise RuntimeError("Model precision {} is not valid".format(model_precision))

    if _HAS_KERAS_TF:
        spec = _convert(
            model=model,
            input_names=input_names,
            output_names=output_names,
            image_input_names=image_input_names,
            is_bgr=is_bgr,
            red_bias=red_bias,
            green_bias=green_bias,
            blue_bias=blue_bias,
            gray_bias=gray_bias,
            image_scale=image_scale,
            class_labels=class_labels,
            predicted_feature_name=predicted_feature_name,
            predicted_probabilities_output=predicted_probabilities_output,
            custom_objects=custom_objects,
            respect_trainable=respect_trainable,
        )
    elif _HAS_KERAS2_TF:
        from . import _keras2_converter

        spec = _keras2_converter._convert(
            model=model,
            input_names=input_names,
            output_names=output_names,
            image_input_names=image_input_names,
            input_name_shape_dict=input_name_shape_dict,
            is_bgr=is_bgr,
            red_bias=red_bias,
            green_bias=green_bias,
            blue_bias=blue_bias,
            gray_bias=gray_bias,
            image_scale=image_scale,
            class_labels=class_labels,
            predicted_feature_name=predicted_feature_name,
            predicted_probabilities_output=predicted_probabilities_output,
            add_custom_layers=add_custom_layers,
            custom_conversion_functions=custom_conversion_functions,
            custom_objects=custom_objects,
            input_shapes=input_shapes,
            output_shapes=output_shapes,
            respect_trainable=respect_trainable,
            use_float_arraytype=use_float_arraytype,
        )
    else:
        raise RuntimeError(
            "Keras not found or unsupported version or backend found. keras conversion API is disabled."
        )

    if model_precision == _MLMODEL_HALF_PRECISION and model is not None:
        spec = _convert_neural_network_spec_weights_to_fp16(spec)

    return spec


def convert(
    model,
    input_names=None,
    output_names=None,
    image_input_names=None,
    input_name_shape_dict={},
    is_bgr=False,
    red_bias=0.0,
    green_bias=0.0,
    blue_bias=0.0,
    gray_bias=0.0,
    image_scale=1.0,
    class_labels=None,
    predicted_feature_name=None,
    model_precision=_MLMODEL_FULL_PRECISION,
    predicted_probabilities_output="",
    add_custom_layers=False,
    custom_conversion_functions=None,
    input_shapes=None,
    output_shapes=None,
    respect_trainable=False,
    use_float_arraytype=False,
):
    """
    Convert a Keras model to Core ML protobuf specification (.mlmodel).

    Parameters
    ----------
    model: Keras model object | str | (str, str)

        A trained Keras neural network model which can be one of the following:

        - a Keras model object
        - a string with the path to a Keras model file (h5)
        - a tuple of strings, where the first is the path to a Keras model
    architecture (.json file), the second is the path to its weights stored in h5 file.

    input_names: [str] | str
        Optional name(s) that can be given to the inputs of the Keras model.
        These names will be used in the interface of the Core ML models to refer
        to the inputs of the Keras model. If not provided, the Keras inputs
        are named to [input1, input2, ..., inputN] in the Core ML model.  When
        multiple inputs are present, the input feature names are in the same
        order as the Keras inputs.

    output_names: [str] | str
        Optional name(s) that can be given to the outputs of the Keras model.
        These names will be used in the interface of the Core ML models to refer
        to the outputs of the Keras model. If not provided, the Keras outputs
        are named to [output1, output2, ..., outputN] in the Core ML model.
        When multiple outputs are present, output feature names are in the same
        order as the Keras inputs.

    image_input_names: [str] | str
        Input names to the Keras model (a subset of the input_names
        parameter) that can be treated as images by Core ML. All other inputs
        are treated as MultiArrays (N-D Arrays).

    is_bgr: bool | dict()
        Flag indicating the channel order the model internally uses to represent
        color images. Set to True if the internal channel order is BGR,
        otherwise it will be assumed RGB. This flag is applicable only if
        image_input_names is specified. To specify a different value for each
        image input, provide a dictionary with input names as keys.
        Note that this flag is about the models internal channel order.
        An input image can be passed to the model in any color pixel layout
        containing red, green and blue values (e.g. 32BGRA or 32ARGB). This flag
        determines how those pixel values get mapped to the internal multiarray
        representation.

    red_bias: float | dict()
        Bias value to be added to the red channel of the input image.
        Defaults to 0.0
        Applicable only if image_input_names is specified.
        To specify different values for each image input provide a dictionary with input names as keys.

    blue_bias: float | dict()
        Bias value to be added to the blue channel of the input image.
        Defaults to 0.0
        Applicable only if image_input_names is specified.
        To specify different values for each image input provide a dictionary with input names as keys.

    green_bias: float | dict()
        Bias value to be added to the green channel of the input image.
        Defaults to 0.0
        Applicable only if image_input_names is specified.
        To specify different values for each image input provide a dictionary with input names as keys.

    gray_bias: float | dict()
        Bias value to be added to the input image (in grayscale). Defaults
        to 0.0
        Applicable only if image_input_names is specified.
        To specify different values for each image input provide a dictionary with input names as keys.

    image_scale: float | dict()
        Value by which input images will be scaled before bias is added and
        Core ML model makes a prediction. Defaults to 1.0.
        Applicable only if image_input_names is specified.
        To specify different values for each image input provide a dictionary with input names as keys.

    class_labels: list[int or str] | str
        Class labels (applies to classifiers only) that map the index of the
        output of a neural network to labels in a classifier.

        If the provided class_labels is a string, it is assumed to be a
        filepath where classes are parsed as a list of newline separated
        strings.

    predicted_feature_name: str
        Name of the output feature for the class labels exposed in the Core ML
        model (applies to classifiers only). Defaults to 'classLabel'

    model_precision: str
        Precision at which model will be saved. Currently full precision (float) and half precision
        (float16) models are supported. Defaults to '_MLMODEL_FULL_PRECISION' (full precision).

    predicted_probabilities_output: str
        Name of the neural network output to be interpreted as the predicted
        probabilities of the resulting classes. Typically the output of a
        softmax function. Defaults to the first output blob.

    add_custom_layers: bool
        If yes, then unknown Keras layer types will be added to the model as
        'custom' layers, which must then be filled in as postprocessing.

    custom_conversion_functions: {str:(Layer -> (dict, [weights])) }
        A dictionary with keys corresponding to names of custom layers and values
        as functions taking a Keras custom layer and returning a parameter dictionary
        and list of weights.

    respect_trainable: bool
        If yes, then Keras layers marked 'trainable' will automatically be
        marked updatable in the Core ML model.

    use_float_arraytype: bool
        If true, the datatype of input/output multiarrays is set to Float32 instead
        of double.

    Returns
    -------
    model: MLModel
    Model in Core ML format.

    Examples
    --------
    .. sourcecode:: python

        # Make a Keras model
        >>> model = Sequential()
        >>> model.add(Dense(num_channels, input_dim = input_dim))

        # Convert it with default input and output names
        >>> import coremltools
        >>> coreml_model = coremltools.converters.keras.convert(model)

        # Saving the Core ML model to a file.
        >>> coreml_model.save('my_model.mlmodel')

    Converting a model with a single image input.

    .. sourcecode:: python

        >>> coreml_model = coremltools.converters.keras.convert(model, input_names =
        ... 'image', image_input_names = 'image')

    Core ML also lets you add class labels to models to expose them as
    classifiers.

    .. sourcecode:: python

        >>> coreml_model = coremltools.converters.keras.convert(model, input_names = 'image',
        ... image_input_names = 'image', class_labels = ['cat', 'dog', 'rat'])

    Class labels for classifiers can also come from a file on disk.

    .. sourcecode:: python

        >>> coreml_model = coremltools.converters.keras.convert(model, input_names =
        ... 'image', image_input_names = 'image', class_labels = 'labels.txt')

    Provide customized input and output names to the Keras inputs and outputs
    while exposing them to Core ML.

    .. sourcecode:: python

        >>> coreml_model = coremltools.converters.keras.convert(model, input_names =
        ...   ['my_input_1', 'my_input_2'], output_names = ['my_output'])

    """
    spec = _convert_to_spec(
        model,
        input_names=input_names,
        output_names=output_names,
        image_input_names=image_input_names,
        input_name_shape_dict=input_name_shape_dict,
        is_bgr=is_bgr,
        red_bias=red_bias,
        green_bias=green_bias,
        blue_bias=blue_bias,
        gray_bias=gray_bias,
        image_scale=image_scale,
        class_labels=class_labels,
        predicted_feature_name=predicted_feature_name,
        model_precision=model_precision,
        predicted_probabilities_output=predicted_probabilities_output,
        add_custom_layers=add_custom_layers,
        custom_conversion_functions=custom_conversion_functions,
        input_shapes=input_shapes,
        output_shapes=output_shapes,
        respect_trainable=respect_trainable,
        use_float_arraytype=use_float_arraytype,
    )

    model = _MLModel(spec)

    from keras import __version__ as keras_version

    model.user_defined_metadata[_METADATA_VERSION] = ct_version
    model.user_defined_metadata[_METADATA_SOURCE] = "keras=={0}".format(keras_version)

    return model
