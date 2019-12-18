# Copyright (c) 2017-2019, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

"""
Neural network builder class to construct Core ML models.
"""

from ... import SPECIFICATION_VERSION
from ... import _MINIMUM_NDARRAY_SPEC_VERSION
from ... import _MINIMUM_UPDATABLE_SPEC_VERSION
from ...proto import Model_pb2 as _Model_pb2
from ...proto import NeuralNetwork_pb2 as _NeuralNetwork_pb2
from ...proto import FeatureTypes_pb2 as _FeatureTypes_pb2
from .._interface_management import set_transform_interface_params, set_training_features
from .. import datatypes
import numpy as np
from .quantization_utils import unpack_to_bytes, _convert_array_to_nbit_quantized_bytes
from .spec_inspection_utils import *
from .update_optimizer_utils import AdamParams, SgdParams

_SUPPORTED_UPDATABLE_LAYERS = ['innerProduct', 'convolution']


def _set_recurrent_activation(param, activation):
    activation = activation.upper() if isinstance(activation, str) else activation
    if activation == 'SIGMOID':
        param.sigmoid.MergeFromString(b'')
    elif activation == 'TANH':
        param.tanh.MergeFromString(b'')
    elif activation == 'LINEAR':
        param.linear.MergeFromString(b'')
    elif activation == 'SIGMOID_HARD':
        param.sigmoidHard.MergeFromString(b'')
    elif activation == 'SCALED_TANH':
        param.scaledTanh.MergeFromString(b'')
    elif activation == 'RELU':
        param.ReLU.MergeFromString(b'')
    else:
        raise TypeError("Unsupported activation type with Recurrent layer: %s." % activation)


def _verify_quantization_arguments(weight=bytes(),
                                   output_channels=1, **kwargs):
    quantization_type = kwargs.get('quantization_type', '').lower()
    nbits = kwargs.get('nbits', 8)
    quant_scale = kwargs.get('quant_scale', None)
    quant_bias = kwargs.get('quant_bias', None)
    quant_lut = kwargs.get('quant_lut', None)

    if not isinstance(weight, bytes):
        raise ValueError('Weight must be of type bytes() for quantization')

    if quantization_type == 'linear':
        if quant_scale is None or quant_bias is None:
            raise ValueError("quant_scale and quant_bias parameters must be provided for linear quantization type")
        if len(quant_scale) != 1 and len(quant_scale) != output_channels:
            raise ValueError("quant_scale should be of type float or an array of length outputChannels")
        if len(quant_bias) != 1 and len(quant_bias) != output_channels:
            raise ValueError("quant_bias should be of type float or an array of length outputChannels")
    elif quantization_type == 'lut':
        if quant_lut is None:
            raise ValueError("quant_lut must be provided for look up table quantization type")
        if len(quant_lut) != 2 ** nbits:
            raise ValueError("quant_lut must be an array of length 2^nbits")
    else:
        raise ValueError("quantization_type must be either linear or lut")

    if quantization_type == 'linear' or 'lut':
        if nbits > 8 or nbits < 1:
            raise ValueError('nbits must be between 1 and 8')


def _fill_quantized_weights(weights_message=None,
                            W=bytes(), **kwargs):
    weights_message.rawValue = bytes()
    weights_message.rawValue += W
    nbits = kwargs.get('nbits', 8)
    weights_message.quantization.numberOfBits = nbits
    quantization_type = kwargs.get('quantization_type', '').lower()
    if quantization_type == 'linear':
        quant_scale = kwargs.get('quant_scale', [1.0])
        quant_bias = kwargs.get('quant_bias', [0.0])
        weights_message.quantization.linearQuantization.scale.extend(map(float, quant_scale))
        weights_message.quantization.linearQuantization.bias.extend(map(float, quant_bias))
    else:
        quant_lut = kwargs.get('quant_lut', [0.0, 1.0])
        weights_message.quantization.lookupTableQuantization.floatValue.extend(map(float, quant_lut))


def _get_nn_spec(spec):
    if spec.HasField('neuralNetworkClassifier'):
        return spec.neuralNetworkClassifier
    elif spec.HasField('neuralNetworkRegressor'):
        return spec.neuralNetworkRegressor
    elif spec.HasField('neuralNetwork'):
        return spec.neuralNetwork
    else:
        return None


def _get_lstm_weight_fields(lstm_wp):
    """
    Get LSTM weight fields.
    lstm_wp: _NeuralNetwork_pb2.LSTMWeightParams
    """
    return [
        lstm_wp.inputGateWeightMatrix,
        lstm_wp.forgetGateWeightMatrix,
        lstm_wp.blockInputWeightMatrix,
        lstm_wp.outputGateWeightMatrix,
        lstm_wp.inputGateRecursionMatrix,
        lstm_wp.forgetGateRecursionMatrix,
        lstm_wp.blockInputRecursionMatrix,
        lstm_wp.outputGateRecursionMatrix,
        lstm_wp.inputGateBiasVector,
        lstm_wp.forgetGateBiasVector,
        lstm_wp.blockInputBiasVector,
        lstm_wp.outputGateBiasVector,
        lstm_wp.inputGatePeepholeVector,
        lstm_wp.forgetGatePeepholeVector,
        lstm_wp.outputGatePeepholeVector]


def _fill_tensor_fields(tensor_field, ranks=None, shapes=None):
    """
    Fill the tensor fields.
    ranks - None or a list of integers with the same length of number of inputs/outputs
    shapes - None or a list of shapes the same length of number of inputs/outputs. Each shape is a list or tuple
    """
    if ranks is None and shapes is None:
        return
    if ranks is None and shapes is not None:
        ranks = [len(shape) for shape in shapes]
    # Fill ranks only
    for rank in ranks:
        if rank is None:
            raise ValueError('Rank of a tensor should not be None')
        # if rank > 5:
        #     raise ValueError('Rank greater than 5 not supported')
        field = tensor_field.add()
        field.rank = rank
    if ranks is not None and shapes is not None:
        # Check validity
        if len(ranks) != len(shapes):
            raise ValueError('Number of rank and shape of tensor field does not match')
        for i, (r, s) in enumerate(zip(ranks, shapes)):
            if s is None:
                continue
            if r != len(s):
                raise ValueError('Rank and shape does not match')
            tensor_field[i].dimValue.extend(s)


class NeuralNetworkBuilder(object):
    """
    Neural network builder class to construct Core ML models.

    The NeuralNetworkBuilder constructs a Core ML neural network specification
    layer by layer. The layers should be added in such an order that the inputs
    to each layer (referred to as blobs) of each layer has been previously
    defined. The builder can also set pre-processing steps to handle
    specialized input format (e.g. images), and set class labels for neural
    network classifiers.
    Refer to the protobuf messages in specification (NeuralNetwork.proto) for more details.

    Examples
    --------
    .. sourcecode:: python

        from coremltools.models.neural_network import datatypes, NeuralNetworkBuilder
        from coremltools.models.utils import save_spec

        # Create a neural network binary classifier that classifies
        # 3-dimensional data points
        # Specify input and output dimensions
        >>> input_dim = (3,)
        >>> output_dim = (2,)

        # Specify input and output features
        >>> input_features = [('data', datatypes.Array(*input_dim))]
        >>> output_features = [('probs', datatypes.Array(*output_dim))]

        # Build a simple neural network with 1 inner product layer
        >>> builder = NeuralNetworkBuilder(input_features, output_features)
        >>> builder.add_inner_product(name='ip_layer', W=weights, b=bias, input_channels=3, output_channels=2,
        ... has_bias=True, input_name='data', output_name='probs')

        # save the spec by the builder
        >>> save_spec(builder.spec, 'network.mlmodel')

    See Also
    --------
    MLModel, datatypes, save_spec
    """

    def __init__(self,
                 input_features=None,
                 output_features=None,
                 mode=None,
                 spec=None,
                 nn_spec=None,
                 disable_rank5_shape_mapping=False,
                 training_features=None):
        """
        Construct a NeuralNetworkBuilder object to build an MLModel specification with
        model interface or a NeuralNetwork protobuf message, either from scratch or an
        existing specification.

        Parameters
        ----------
        input_features: [(str, datatypes.Array)] or None
            List of input feature of the network. Each feature is a (name,
            array) tuple, where name the name of the feature, and array
            is an datatypes.Array object describing the feature type.
            When spec is None (building from scratch), input_features must not be None.
            When spec is not None, input_features will be ignored; input feature of
            existing spec will be used instead.

        output_features: [(str, datatypes.Array or None)] or None
            List of output feature of the network. Each feature is a (name,
            array) tuple, where name is the name of the feature, and array
            is an datatypes.Array object describing the feature type.
            array can be None if not known.
            When spec is None (building from scratch), output_features must not be None.
            When spec is not None, output_features will be ignored; output feature of
            existing spec will be used instead.

        mode: str ('classifier', 'regressor' or None)
            Mode (one of 'classifier', 'regressor', or None).

            When mode = 'classifier', a NeuralNetworkClassifier spec will be
            constructed.  When mode = 'regressor', a NeuralNetworkRegressor
            spec will be constructed.

        disable_rank5_shape_mapping: bool
            Only applicable for neural networks.
            If True, inputs are no longer forced to map to rank 5 tensors
            (rank is equal to the length of the shape of the tensor).
            Instead, for multi-array inputs "EXACT_ARRAY_MAPPING" mapping is used, whereas
            for image inputs "RANK4_IMAGE_MAPPING" is used.
            For details, see description of enums "NeuralNetworkMultiArrayShapeMapping"
            and "NeuralNetworkImageShapeMapping" in NeuralNetwork.proto.
            When spec is not None, this argument will be ignored.

        spec: None or coremltools.proto.Model_pb2
            If None, a new MLModel spec will be created by the builder with input and output features.
            Otherwise, the builder will continue to build on spec. This is useful when the MLModel is
            built incrementally.

        nn_spec: None or coremltools.proto.NeuralNetwork_pb2
            If None, a new, empty NeuralNetwork proto will be created for spec.
            If nn_spec is not None and spec is None, the builder will build a NeuralNetwork spec without
            wrapping it within an MLModel. This is useful to create nested NeuralNetworks for models
            with control flow operations.

        Examples
        --------
        .. sourcecode:: python

            # Construct a builder that builds a neural network classifier with a 299 x 299 x 3
            # dimensional input and 1000 dimensional output
            >>> input_features = [('data', datatypes.Array((299, 299, 3)))]
            >>> output_features = [('probs', datatypes.Array((1000,)))]
            >>> builder = NeuralNetworkBuilder(input_features, output_features, mode='classifier')

        See Also
        --------
        set_input, set_output, set_class_labels
        """
        self.spec = spec
        self.nn_spec = nn_spec
        self._disable_rank5_shape_mapping = disable_rank5_shape_mapping
        self.layers = []
        self.layer_specs = {}
        self.named_parameters = []
        self.rank_dict = {}

        if self.spec is not None:  # Existing spec
            if self.nn_spec is None:
                self.nn_spec = _get_nn_spec(self.spec)
                for layer_spec in self.nn_spec.layers:
                    self.layers.append(layer_spec.name)
                    self.layer_specs[layer_spec.name] = layer_spec
            else:
                # Both spec and nn_spec are not None
                raise ValueError('Attempting to assign another NeuralNetwork Spec to an existing MLModel Spec')
            if input_features is None and output_features is None:
                return

        if self.spec is None and self.nn_spec is not None:  # Building nested Neural Network
            return

        # Set the interface params.
        if self.spec is None:
            self.spec = _Model_pb2.Model()
        self.spec.specificationVersion = SPECIFICATION_VERSION
        if disable_rank5_shape_mapping:
            self.spec.specificationVersion = _MINIMUM_NDARRAY_SPEC_VERSION

        # When output_features in None, use some dummy sized type
        out_features_with_shape = []
        for out_feature in output_features:
            feat_name, feat_type = out_feature
            if feat_type is None:
                out_features_with_shape.append((str(feat_name), datatypes.Array(1)))
            else:
                out_features_with_shape.append(out_feature)

        # Set interface inputs and outputs
        if len(self.spec.description.input) > 0:
            del self.spec.description.input[:]
        if len(self.spec.description.output) > 0:
            del self.spec.description.output[:]

        self.spec = set_transform_interface_params(self.spec, input_features,
                                                   out_features_with_shape, training_features=training_features)

        for input in input_features:
            self.rank_dict[input[0]] = len(input[1].dimensions)

        for idx, output_feature in enumerate(output_features):
            if output_features[idx][1] is None:
                self.spec.description.output[idx].type.multiArrayType.ClearField("shape")

        if self.nn_spec is None:
            if mode == 'classifier':
                nn_spec = self.spec.neuralNetworkClassifier
            elif mode == 'regressor':
                nn_spec = self.spec.neuralNetworkRegressor
            else:
                nn_spec = self.spec.neuralNetwork
            self.nn_spec = nn_spec

        if disable_rank5_shape_mapping and self.nn_spec:
            self.nn_spec.arrayInputShapeMapping = _NeuralNetwork_pb2.NeuralNetworkMultiArrayShapeMapping.Value(
                'EXACT_ARRAY_MAPPING')
            self.nn_spec.imageInputShapeMapping = _NeuralNetwork_pb2.NeuralNetworkImageShapeMapping.Value(
                'RANK4_IMAGE_MAPPING')

    def set_input(self, input_names, input_dims):
        """
        Set the inputs of the network spec.

        Parameters
        ----------
        input_names: list of str
            The input names of the network.

        input_dims: [tuple]
            The input dimensions of the network. The ordering of input_dims
            is the same as input_names.

        Examples
        --------
        .. sourcecode:: python

            # Set the neural network spec inputs to be 3 dimensional vector data1 and
            # 4 dimensional vector data2.
            >>> builder.set_input(input_names=['data1', 'data2'], input_dims=[(3,), (4,)])

        See Also
        --------
        set_output, set_class_labels
        """

        if len(input_names) != len(input_dims):
            raise ValueError('input_names and input_dims must be of the same sizes.')

        spec = self.spec
        for idx, dim in enumerate(input_dims):
            if hasattr(self, '_disable_rank5_shape_mapping') and self._disable_rank5_shape_mapping:
                input_shape = dim
            else:
                if len(dim) == 3:
                    input_shape = (dim[0], dim[1], dim[2])
                elif len(dim) == 2:
                    input_shape = (dim[1],)
                elif len(dim) == 1:
                    input_shape = tuple(dim)
                else:
                    raise RuntimeError("Attempting to add a neural network " +
                                       "input with rank " + str(len(dim)) +
                                       ". All networks should take inputs of rank 1 or 3.")

            spec.description.input[idx].type.multiArrayType.ClearField("shape")
            spec.description.input[idx].type.multiArrayType.shape.extend(input_shape)

            # TODO: if it's an embedding, this should be integer
            spec.description.input[idx].type.multiArrayType.dataType = _Model_pb2.ArrayFeatureType.DOUBLE

            spec.description.input[idx].name = input_names[idx]

    def set_output(self, output_names, output_dims):
        """
        Set the outputs of the network spec.

        Parameters
        ----------
        output_names: list of str
            The output names of the network.

        output_dims: [tuple]
            The output dimensions of the network. The ordering of output_dims is the same
            as output_names.

        Examples
        --------
        .. sourcecode:: python

            # Set the neural network spec outputs to be 3 dimensional vector feature1 and
            # 4 dimensional vector feature2.
            >>> builder.set_output(output_names=['feature1', 'feature2'], output_dims=[(3,), (4,)])

        See Also
        --------
        set_input, set_class_labels
        """

        if len(output_names) != len(output_dims):
            raise ValueError('output_names and output_dims must be of the same sizes.')

        spec = self.spec
        for idx, dim in enumerate(output_dims):
            spec.description.output[idx].type.multiArrayType.ClearField("shape")
            spec.description.output[idx].type.multiArrayType.shape.extend(dim)
            spec.description.output[idx].type.multiArrayType.dataType = \
                _Model_pb2.ArrayFeatureType.DOUBLE

            spec.description.output[idx].name = output_names[idx]

    def set_training_input(self, training_input):
        """
        Set the training inputs of the network spec.

        Parameters
        ----------
        training_input: [tuple]
            The training input names and type of the network.

        Examples
        --------
        .. sourcecode:: python

            # Set the neural network spec training inputs to be 3 dimensional vector for 'input' and
            # Double for 'target'.
            >>> builder.set_training_input([('input', datatypes.Array(3)), ('target', 'Double')])
        """
        spec = self.spec
        set_training_features(spec, training_input)

    def set_class_labels(self, class_labels, predicted_feature_name='classLabel', prediction_blob=''):
        """
        Set class labels to the model spec to make it a neural network classifier.

        Parameters
        ----------
        class_labels: list of int or list of str
            A list of integers or strings that map the index of the output of a
            neural network to labels in a classifier.

        predicted_feature_name: str
            Name of the output feature for the class labels exposed in the
            Core ML neural network classifier, defaults: 'class_output'.

        prediction_blob: str
            If provided, then this is the name of the neural network blob which
            generates the probabilities for each class label (typically the output
            of a softmax layer). If not provided, then the last output layer is
            assumed.

        See Also
        --------
        set_input, set_output, set_pre_processing_parameters
        """
        spec = self.spec
        nn_spec = self.nn_spec

        if len(spec.description.output) == 0:
            raise ValueError(
                "Model should have at least one output (the probabilities) to automatically make it a classifier.")
        probOutput = spec.description.output[0]
        probOutput.type.dictionaryType.MergeFromString(b'')
        if len(class_labels) == 0:
            return
        class_type = type(class_labels[0])
        if class_type not in [int, str]:
            raise TypeError("Class labels must be of type Integer or String. (not %s)" % class_type)

        spec.description.predictedProbabilitiesName = probOutput.name
        spec.description.predictedFeatureName = predicted_feature_name

        classLabel = spec.description.output.add()
        classLabel.name = predicted_feature_name
        if class_type == int:
            nn_spec.ClearField('int64ClassLabels')
            probOutput.type.dictionaryType.int64KeyType.MergeFromString(b'')
            classLabel.type.int64Type.MergeFromString(b'')
            for c in class_labels:
                nn_spec.int64ClassLabels.vector.append(c)
        else:
            nn_spec.ClearField('stringClassLabels')
            probOutput.type.dictionaryType.stringKeyType.MergeFromString(b'')
            classLabel.type.stringType.MergeFromString(b'')
            for c in class_labels:
                nn_spec.stringClassLabels.vector.append(c)

        if prediction_blob != '':
            # correctness here will be checked in the validator -- i.e. to
            # make sure this string corresponds to a real blob
            nn_spec.labelProbabilityLayerName = prediction_blob
        else:  # not provided
            # assume it's the last blob produced in the network
            nn_spec.labelProbabilityLayerName = nn_spec.layers[-1].output[0]

    def add_optionals(self, optionals_in, optionals_out):
        """
        Add optional inputs and outputs to the model spec.

        Parameters
        ----------
        optionals_in: list of str
            List of inputs that are optionals.

        optionals_out: list of str
            List of outputs that are optionals.

        See Also
        --------
        set_input, set_output

        """
        spec = self.spec
        if (not optionals_in) and (not optionals_out):
            return

        input_types = [datatypes.Array(dim) if isinstance(dim, int) else datatypes.Array(*dim) for (name, dim) in optionals_in]
        output_types = []
        for name, dim in optionals_out:
            if not dim:
                output_types.append(None)
            elif isinstance(dim, int):
                output_types.append(datatypes.Array(dim))
            else:
                output_types.append(datatypes.Array(*dim))

        input_names = [str(name) for (name, dim) in optionals_in]
        output_names = [str(name) for (name, dim) in optionals_out]

        input_features = list(zip(input_names, input_types))
        output_features = list(zip(output_names, output_types))

        len_before_in = len(spec.description.input)
        len_before_out = len(spec.description.output)

        # this appends to the existing model interface
        set_transform_interface_params(spec, input_features, output_features, True)

        # add types for any extra hidden inputs
        for idx in range(len_before_in, len(spec.description.input)):
            spec.description.input[idx].type.multiArrayType.dataType = _Model_pb2.ArrayFeatureType.DOUBLE
        for idx in range(len_before_out, len(spec.description.output)):
            spec.description.output[idx].type.multiArrayType.dataType = _Model_pb2.ArrayFeatureType.DOUBLE

    def make_updatable(self, trainables):
        """
        Make the builder's NeuralNetwork spec updatable.

        Parameters
        ----------
        trainables: list of str
            List of layer names to be set trainable.
        """
        if self.spec is None:
            return
        self.spec.isUpdatable = True

        if not self.spec.specificationVersion or self.spec.specificationVersion < _MINIMUM_UPDATABLE_SPEC_VERSION:
            self.spec.specificationVersion = _MINIMUM_UPDATABLE_SPEC_VERSION

        self.nn_spec.updateParams.MergeFromString(b'')
        self.set_shuffle()

        for trainable in trainables:
            if trainable not in self.layer_specs:
                raise ValueError('Layer %s does not exist.' % trainable)
            spec_layer = self.layer_specs[trainable]
            spec_layer_type = spec_layer.WhichOneof('layer')
            if spec_layer_type not in _SUPPORTED_UPDATABLE_LAYERS:
                raise ValueError('Layer %s is not supported to be marked as updatable. Only %s layers '
                                 'are supported to be marked updatable.' % (trainable, _SUPPORTED_UPDATABLE_LAYERS))
            spec_layer.isUpdatable = True
            typed_layer = getattr(spec_layer, spec_layer.WhichOneof('layer'))
            for fd in typed_layer.DESCRIPTOR.fields:
                field = getattr(typed_layer, fd.name)
                if type(field) == _NeuralNetwork_pb2.LSTMWeightParams:
                    wfs = _get_lstm_weight_fields(field)
                    for wf in wfs:
                        wf.isUpdatable = True
                elif type(field) == _NeuralNetwork_pb2.WeightParams:
                    field.isUpdatable = True
                else:
                    pass

    def set_categorical_cross_entropy_loss(self, name, input):
        """
        Categorical Cross Entropy is used for single label categorization (only one category is applicable for each data point).

        Parameters
        ----------
        name: The name of the loss layer
        input: The name of the input, which should be a vector of length N representing the distribution over N categories. This must be the output of a softmax.

        .. math::
        Loss_ {CCE}(input, target) = -\sum_{i = 1} ^ {N}(target == i) log(input[i]) = - log(input[target])
        """
        if self.spec is None:
            return

        if name in self.layer_specs:
            raise ValueError("Name %s is already used." % name)

        if input is None:
            raise ValueError('Loss Layer input must be specified')

        target = input + '_true'

        if len(self.nn_spec.layers) < 1:
            raise ValueError('Loss layer (%s) cannot be attached to an empty model.' % name)

        # validate input
        # input must be a softmax layer output
        input_validated = False
        for _, layer in enumerate(self.nn_spec.layers[::-1]):
            layer_outputs = list(layer.output)
            layer_type = layer.WhichOneof('layer')

            if input in layer_outputs and layer_type == 'softmax':
                input_validated = True
                break

        if not input_validated:
            raise ValueError('Categorical Cross Entropy loss layer input (%s) must be a softmax layer output.' % input)

        # validate target
        output_names = [x.name for x in self.spec.description.output]
        if target in output_names:
            raise ValueError('Loss layer target (%s) must not be a model output.' % target)

        updating_classifier = False
        predicted_probabilities_name = self.spec.description.predictedProbabilitiesName
        predicted_feature_name = self.spec.description.predictedFeatureName
        if self.spec.HasField('neuralNetworkClassifier') and input == predicted_probabilities_name:
            updating_classifier = True

        loss_layer = self.nn_spec.updateParams.lossLayers.add()
        self.layers.append(name)
        self.layer_specs[name] = loss_layer
        loss_layer.name = name
        loss_layer.categoricalCrossEntropyLossLayer.input = input
        loss_layer.categoricalCrossEntropyLossLayer.target = target

        classifier_output = self.spec.description.predictedFeatureName

        training_inputs = self.spec.description.trainingInput
        training_inputs.extend(self.spec.description.input)
        training_input = training_inputs.add()

        if updating_classifier:
            training_input.name = predicted_feature_name
            classifier_output_type = [x.type for x in self.spec.description.output if x.name == predicted_feature_name]

            model_type = classifier_output_type[0].WhichOneof('Type')
            if model_type == 'stringType':
                datatypes._set_datatype(training_input.type, datatypes.String())
            elif model_type == 'int64Type':
                datatypes._set_datatype(training_input.type, datatypes.Int64())
        else:
            training_input.name = target
            datatypes._set_datatype(training_input.type, datatypes.Array(1))
            training_input.type.multiArrayType.dataType = _Model_pb2.ArrayFeatureType.INT32

        print('Now adding input {} as target for categorical cross-entropy loss layer.'.format(target))

    def set_mean_squared_error_loss(self, name, input_feature=None):
        """
        input_feature: [(str, datatypes.Array)] or None
            The input feature of the loss layer. Each feature is a (name,
            array) tuple, where name is the name of the model's tensor our loss will be attached to,
            and array is a datatypes.Array object describing the shape of that tensor.
            Both the name and the array's shape must be provided in the tuple.
            >>> feature = [('output_tensor', datatypes.Array((299, 299, 3)))]
        """
        if self.spec is None:
            return

        if name in self.layer_specs:
            raise ValueError("Name %s is already used." % name)

        if input_feature is None:
            raise ValueError('Loss Layer input must be specified')

        if not isinstance(input_feature, tuple):
            raise ValueError('Loss layer input must be a tuple of type (string, datatype)')

        (fname, ftype) = input_feature
        if not isinstance(fname, str):
            raise ValueError('Loss layer input must be a tuple of type (string, datatype)')
        if not isinstance(ftype, datatypes.Array):
            raise ValueError('Loss layer input must be a tuple of type (string, datatype)')

        target = fname + '_true'

        loss_layer = self.nn_spec.updateParams.lossLayers.add()
        self.layers.append(name)
        self.layer_specs[name] = loss_layer
        loss_layer.name = name

        output_names = [x.name for x in self.spec.description.output]
        if target in output_names:
            raise ValueError('Loss Layer target (%s) must not be a model output' % target)

        loss_layer.meanSquaredErrorLossLayer.input = input_feature[0]
        loss_layer.meanSquaredErrorLossLayer.target = target

        training_inputs = self.spec.description.trainingInput
        training_inputs.extend(self.spec.description.input)
        training_input = training_inputs.add()
        training_input.name = target

        datatypes._set_datatype(training_input.type, input_feature[1])
        training_input.type.multiArrayType.dataType = _Model_pb2.ArrayFeatureType.DOUBLE
        print('Now adding input {} as target for mean squared error loss layer.'.format(target))

    def set_sgd_optimizer(self, sgd_params):
        if self.spec is None:
            return

        if not isinstance(sgd_params, SgdParams):
            raise Exception('sgd_params must be of instance SgdParams')

        sgd_optimizer = self.nn_spec.updateParams.optimizer.sgdOptimizer

        # set learning rate
        sgd_optimizer.learningRate.defaultValue = sgd_params.lr.value
        sgd_optimizer.learningRate.range.minValue = sgd_params.lr.min
        sgd_optimizer.learningRate.range.maxValue = sgd_params.lr.max

        # set mini batch size
        sgd_optimizer.miniBatchSize.defaultValue = sgd_params.batch.value
        sgd_optimizer.miniBatchSize.set.values.extend(sgd_params.batch.allowed_set)

        # set momentum
        sgd_optimizer.momentum.defaultValue = sgd_params.momentum.value
        sgd_optimizer.momentum.range.minValue = sgd_params.momentum.min
        sgd_optimizer.momentum.range.maxValue = sgd_params.momentum.max

    def set_adam_optimizer(self, adam_params):
        if self.spec is None:
            return

        if not isinstance(adam_params, AdamParams):
            raise Exception('adam_params must be of instance AdamParams')

        adam_optimizer = self.nn_spec.updateParams.optimizer.adamOptimizer

        # set learning rate
        adam_optimizer.learningRate.defaultValue = adam_params.lr.value
        adam_optimizer.learningRate.range.minValue = adam_params.lr.min
        adam_optimizer.learningRate.range.maxValue = adam_params.lr.max

        # set mini batch size
        adam_optimizer.miniBatchSize.defaultValue = adam_params.batch.value
        adam_optimizer.miniBatchSize.set.values.extend(adam_params.batch.allowed_set)

        # set beta1
        adam_optimizer.beta1.defaultValue = adam_params.beta1.value
        adam_optimizer.beta1.range.minValue = adam_params.beta1.min
        adam_optimizer.beta1.range.maxValue = adam_params.beta1.max

        # set beta2
        adam_optimizer.beta2.defaultValue = adam_params.beta2.value
        adam_optimizer.beta2.range.minValue = adam_params.beta2.min
        adam_optimizer.beta2.range.maxValue = adam_params.beta2.max

        # set eps
        adam_optimizer.eps.defaultValue = adam_params.eps.value
        adam_optimizer.eps.range.minValue = adam_params.eps.min
        adam_optimizer.eps.range.maxValue = adam_params.eps.max

    def set_epochs(self, epochs=1, allowed_set=None):
        if self.spec is None:
            return

        self.nn_spec.updateParams.epochs.defaultValue = epochs

        if allowed_set is None:
            self.nn_spec.updateParams.epochs.set.values.extend([epochs])
        else:
            self.nn_spec.updateParams.epochs.set.values.extend(allowed_set)

    def set_shuffle(self, seed=None):
        if self.spec is None:
            return

        # Validate that seed passed in is integer
        if seed is not None:
            if not isinstance(seed, int):
                raise TypeError('Shuffle seed value must be integer')

        self.nn_spec.updateParams.shuffle.defaultValue = True
        if seed is not None:
            self.nn_spec.updateParams.seed.defaultValue = seed

    def _add_generic_layer(self, name, input_names, output_names,
                           input_ranks=None, input_shapes=None,
                           output_ranks=None, output_shapes=None):
        generic_layer = self.nn_spec.layers.add()
        generic_layer.name = name
        generic_layer.input.extend(input_names)
        generic_layer.output.extend(output_names)
        self.layers.append(name)
        if name in self.layer_specs:
            raise ValueError('Layer with name \"%s\" has already been added. Please use a unique name.' % name)
        self.layer_specs[name] = generic_layer
        _fill_tensor_fields(generic_layer.inputTensor, input_ranks, input_shapes)
        _fill_tensor_fields(generic_layer.outputTensor, output_ranks, output_shapes)

        # Pass Rank Information
        # Generic Layer copies rank of first input to all of its output
        # All the layers that modifies rank apart from first input must override
        if input_names is not None and len(input_names) > 0:
            for output_ in output_names:
                self.rank_dict[output_] = self._get_rank(input_names[0])
        return generic_layer

    def inspect_layers(self, last=-1, verbose=False):
        """ Prints the summary for last "last" number of layers.

        Parameters
        ----------
        last: int
             The numbers of layers to inspect, starting from the last one.
        verbose: bool
            Whether to display layer-specific parameters or not
        """
        n_layers = len(self.nn_spec.layers)
        if last < 0:
            last = n_layers
        for i, alayer in enumerate(self.nn_spec.layers[::-1]):
            if i >= last:
                break
            layer_type, name, in_blobs, out_blobs, params_info = summarize_network_layer_info(alayer)
            print('[Id: {}], Name: {} (Type: {})'.format(n_layers - i - 1, name, layer_type))
            print(' ' * 10 + 'Updatable: {}'.format(alayer.isUpdatable))
            print(' ' * 10 + 'Input blobs: {}'.format(in_blobs))
            print(' ' * 10 + 'Output blobs: {}'.format(out_blobs))
            if verbose and len(params_info) > 0:
                print(' ' * 10 + 'Parameters: ')
                for param in params_info:
                    print(' ' * 14 + '{} = {}'.format(param[0], param[1]))

    def inspect_loss_layers(self):
        """ Prints the summary for the loss layer.
        """
        n_loss_layers = len(self.nn_spec.updateParams.lossLayers)
        if n_loss_layers < 1:
            print('no loss layer detected.')
        for i, loss_layer in enumerate(self.nn_spec.updateParams.lossLayers[::-1]):
            loss_type = loss_layer.WhichOneof('LossLayerType')
            loss_name = loss_layer.name
            loss_input = None
            loss_target = None
            if loss_type == 'categoricalCrossEntropyLossLayer':
                loss_input = loss_layer.categoricalCrossEntropyLossLayer.input
                loss_target = loss_layer.categoricalCrossEntropyLossLayer.target
            elif loss_type == 'meanSquaredErrorLossLayer':
                loss_input = loss_layer.meanSquaredErrorLossLayer.input
                loss_target = loss_layer.meanSquaredErrorLossLayer.target

            print('[Id: {}], Name: {} (Type: {})'.format(n_loss_layers - i - 1, loss_name, loss_type))
            print(' ' * 10 + 'Loss Input: {}'.format(loss_input))
            print(' ' * 10 + 'Loss Target: {}'.format(loss_target))

    def inspect_optimizer(self):
        """ Prints the summary for the optimizer.
        """
        optimizer = self.nn_spec.updateParams.optimizer
        optimizer_type = optimizer.WhichOneof('OptimizerType')
        print('Optimizer Type: {}'.format(optimizer_type))
        if optimizer_type == 'sgdOptimizer':
            lr = optimizer.sgdOptimizer.learningRate
            batch = optimizer.sgdOptimizer.miniBatchSize
            momentum = optimizer.sgdOptimizer.momentum
            print('lr: {}, min: {}, max: {}'.format(lr.defaultValue, lr.range.minValue, lr.range.maxValue))
            print('batch: {}, allowed_set: {}'.format(batch.defaultValue, batch.set.values))
            print('momentum: {}, min: {}, max: {}'.format(momentum.defaultValue, momentum.range.minValue, momentum.range.maxValue))
        elif optimizer_type == 'adamOptimizer':
            lr = optimizer.adamOptimizer.learningRate
            batch = optimizer.adamOptimizer.miniBatchSize
            beta1 = optimizer.adamOptimizer.beta1
            beta2 = optimizer.adamOptimizer.beta2
            eps = optimizer.adamOptimizer.eps
            print('lr: {}, min: {}, max: {}'.format(lr.defaultValue, lr.range.minValue, lr.range.maxValue))
            print('batch: {}, allowed_set: {}'.format(batch.defaultValue, batch.set.values))
            print('beta1: {}, min: {}, max: {}'.format(beta1.defaultValue, beta1.range.minValue, beta1.range.maxValue))
            print('beta2: {}, min: {}, max: {}'.format(beta2.defaultValue, beta2.range.minValue, beta2.range.maxValue))
            print('epsilon: {}, min: {}, max: {}'.format(eps.defaultValue, eps.range.minValue, eps.range.maxValue))

    def inspect_updatable_layers(self):
        """ Prints all updatable layers with their inputs and outputs.
        """
        for _, layer in enumerate(self.nn_spec.layers[::-1]):
            if layer.isUpdatable:
                layer_type, name, in_blobs, out_blobs, _ = summarize_network_layer_info(layer)
                print('Name: {} (Type: {})'.format(name, layer_type))
                print(' ' * 10 + 'Input blobs: {}'.format(in_blobs))
                print(' ' * 10 + 'Output blobs: {}'.format(out_blobs))

    def inspect_input_features(self):
        """ Prints the name and type of input features.
        """
        input_features = self.spec.description.input
        n_input_features = len(input_features)
        if n_input_features < 1:
            return
        for i, input_feature in enumerate(input_features[::-1]):
            print('[Id: {}] Name: {}'.format(n_input_features - i - 1, input_feature.name))
            print(' ' * 10 + 'Type: {}'.format(input_feature.type))

    def inspect_output_features(self):
        """ Prints the name and type of output features.
        """
        output_features = self.spec.description.output
        n_output_features = len(output_features)
        if n_output_features < 1:
            return
        for i, output_feature in enumerate(output_features[::-1]):
            print('[Id: {}] Name: {}'.format(n_output_features - i - 1, output_feature.name))
            print(' ' * 10 + 'Type: {}'.format(output_feature.type))

    def inspect_conv_channels(self, layer_name):
        """ Prints the output and kernel channels of a convolution layer.
        """
        if self.spec is None:
            return
        if layer_name not in self.layer_specs:
            raise ValueError('Layer %s does not exist.' % (layer_name))
        spec_layer = self.layer_specs[layer_name]

        if spec_layer.WhichOneof('layer') != 'convolution':
            raise ValueError('Layer %s is not a convolution layer.' % (layer_name))

        output_channels = spec_layer.convolution.outputChannels
        kernel_channels = spec_layer.convolution.kernelChannels
        print('outputChannels: {}'.format(output_channels))
        print('kernelChannels: {}'.format(kernel_channels))

    def inspect_innerproduct_channels(self, layer_name):
        """ Prints the output and kernel channels of an innerProduct layer.
        """
        if self.spec is None:
            return
        if layer_name not in self.layer_specs:
            raise ValueError('Layer %s does not exist.' % (layer_name))
        spec_layer = self.layer_specs[layer_name]

        if spec_layer.WhichOneof('layer') != 'innerProduct':
            raise ValueError('Layer %s is not an innerProduct layer.' % (layer_name))

        input_channels = spec_layer.innerProduct.inputChannels
        output_channels = spec_layer.innerProduct.outputChannels
        print('inputChannels: {}'.format(input_channels))
        print('outputChannels: {}'.format(output_channels))

    def _get_rank(self, name):
        return self.rank_dict[name] if name in self.rank_dict else -1

    def _set_max_input_rank(self, input_names, output_name):
        if len(input_names) == 0:
            raise ValueError('Input name list empty for collecting rank information')
        self.rank_dict[output_name] = -1
        for i in range(0, len(input_names)):
            input_rank = self._get_rank(input_names[i])
            if input_rank == -1:
                self.rank_dict[output_name] = -1
                return
            self.rank_dict[output_name] = max(self._get_rank(output_name), input_rank)

    def _set_rank_for_reduce_op(self, input_name, output_name, axes, keepdims, reduce_all):
        if keepdims:
            self.rank_dict[output_name] = self._get_rank(input_name)
        else:
            if reduce_all or self._get_rank(input_name) == 1:
                self.rank_dict[output_name] = 1
            elif axes and len(axes) > 0:
                rank = self._get_rank(input_name) - len(axes)
                self.rank_dict[output_name] = rank if rank != 0 else 1
            else:
                raise ValueError('Reduce Ops must provide axes to reduce on if reduce_all is False')

    def add_inner_product(self, name, W, b, input_channels, output_channels, has_bias,
                          input_name, output_name, **kwargs):
        """
        Add an inner product layer to the model.
        Refer to the **InnerProductLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        W: numpy.array or bytes()
            Weight matrix of shape (output_channels, input_channels)
            If W is of type bytes(), i.e. quantized, other quantization related arguments must be provided as well (see below).
        b: numpy.array
            Bias vector of shape (output_channels, ).
        input_channels: int
            Number of input channels.
        output_channels: int
            Number of output channels.
        has_bias: boolean
            Whether the bias vector of this layer is ignored in the spec.

            - If True, the bias vector of this layer is not ignored.
            - If False, the bias vector is ignored.

        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        Quantization arguments expected in kwargs, when W is of type bytes():

            quantization_type: str
                When weights are quantized (i.e. W is of type bytes()), this should be either "linear" or "lut".

            nbits: int
                Should be between 1 and 8 (inclusive). Number of bits per weight value. Only applicable when
                weights are quantized.

            quant_scale: numpy.array(dtype=numpy.float32)
                scale vector to be used with linear quantization. Must be of length either 1 or output_channels.

            quant_bias: numpy.array(dtype=numpy.float32)
                bias vector to be used with linear quantization. Must be of length either 1 or output_channels.

            quant_lut: numpy.array(dtype=numpy.float32)
                the LUT (look up table) to be used with LUT quantization. Must be of length 2^nbits.

        See Also
        --------
        add_embedding, add_convolution
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.innerProduct

        # Fill in the parameters
        spec_layer_params.inputChannels = input_channels
        spec_layer_params.outputChannels = output_channels
        spec_layer_params.hasBias = has_bias

        weights = spec_layer_params.weights
        if len(kwargs) == 0:
            weights.floatValue.extend(map(float, W.flatten()))
        else:
            _verify_quantization_arguments(weight=W, output_channels=output_channels, **kwargs)
            _fill_quantized_weights(weights_message=weights, W=W, **kwargs)

        if has_bias:
            bias = spec_layer_params.bias
            bias.floatValue.extend(map(float, b.flatten()))
        return spec_layer

    def add_embedding(self, name, W, b, input_dim, output_channels, has_bias,
                      input_name, output_name,
                      is_quantized_weight=False,
                      quantization_type='linear',
                      nbits=8,
                      quant_scale=None,
                      quant_bias=None,
                      quant_lut=None):
        """
        Add an embedding layer to the model.
        Refer to the **EmbeddingLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        W: float32 numpy.array or bytes()
            Weight matrix of shape (output_channels, input_dim).
            If W is of type bytes(), i.e. quantized to 1-8 bits, other quantization related arguments must be provided as well (see below).
        b: numpy.array
            Bias vector of shape (output_channels, ).
        input_dim: int
            Size of the vocabulary (1 + maximum integer index of the words).
        output_channels: int
            Number of output channels.
        has_bias: boolean
            Whether the bias vector of this layer is ignored in the spec.

            - If True, the bias vector of this layer is not ignored.
            - If False, the bias vector is ignored.

        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.


        Quantization arguments expected, when W is of type bytes():

        is_quantized_weight: bool
            Set it to true when W is of type bytes(), representing quantized weights

        quantization_type: str
            When weights are quantized (i.e. W is of type bytes()), this should be either "linear" or "lut".

        nbits: int
            Should be between 1 and 8 (inclusive). Number of bits per weight value.

        quant_scale: numpy.array(dtype=numpy.float32)
            scale vector to be used with linear quantization. Must be of length either 1 or output_channels.

        quant_bias: numpy.array(dtype=numpy.float32)
            bias vector to be used with linear quantization. Must be of length either 1 or output_channels.

        quant_lut: numpy.array(dtype=numpy.float32)
            the LUT (look up table) to be used with LUT quantization. Must be of length 2^nbits.

        See Also
        --------
        add_inner_product
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])

        # Fill in the parameters
        spec_layer_params = spec_layer.embedding

        spec_layer_params.inputDim = input_dim
        spec_layer_params.outputChannels = output_channels
        spec_layer_params.hasBias = has_bias

        weights = spec_layer_params.weights
        if not is_quantized_weight:
            weights.floatValue.extend(map(float, W.flatten()))
        else:
            _verify_quantization_arguments(weight=W, output_channels=output_channels,
                                           quantization_type=quantization_type, nbits=nbits,
                                           quant_scale=quant_scale, quant_bias=quant_bias, quant_lut=quant_lut)

            _fill_quantized_weights(weights_message=weights, W=W,
                                    quantization_type=quantization_type, nbits=nbits,
                                    quant_scale=quant_scale, quant_bias=quant_bias, quant_lut=quant_lut)

        if has_bias:
            bias = spec_layer_params.bias
            bias.floatValue.extend(map(float, b.flatten()))

        return spec_layer

    def add_softmax(self, name, input_name, output_name):
        """
        Add a softmax layer to the model.
        Refer to the **SoftmaxLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_activation, add_inner_product, add_convolution
        """
        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.softmax.MergeFromString(b'')
        return spec_layer

    def add_activation(self, name, non_linearity, input_name, output_name,
                       params=None):
        """
        Add an activation layer to the model.
        Refer to the specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        non_linearity: str
            The non_linearity (activation) function of this layer.
            It can be one of the following:

                - 'RELU': Rectified Linear Unit (ReLU) function.
                - 'SIGMOID': sigmoid function.
                - 'TANH': tanh function.
                - 'SCALED_TANH': scaled tanh function, defined as:

                  ``f(x) = alpha * tanh(beta * x)``

                  where alpha and beta are constant scalars.

                - 'SOFTPLUS': softplus function.
                - 'SOFTSIGN': softsign function.
                - 'SIGMOID_HARD': hard sigmoid function, defined as:

                  ``f(x) = min(max(alpha * x + beta, -1), 1)``

                  where alpha and beta are constant scalars.
                - 'LEAKYRELU': leaky relu function, defined as:

                  ``f(x) = (x >= 0) * x + (x < 0) * alpha * x``

                  where alpha is a constant scalar.
                - 'PRELU': Parametric ReLU function, defined as:

                  ``f(x) = (x >= 0) * x + (x < 0) * alpha * x``

                  where alpha is a multi-dimensional array of same size as x.
                - 'ELU': Exponential linear unit function, defined as:

                  ``f(x) = (x >= 0) * x + (x < 0) * (alpha * exp(x) - 1)``

                  where alpha is a constant scalar.

                - 'PARAMETRICSOFTPLUS': Parametric softplus function, defined as:

                  ``f(x) = alpha * log(1 + exp(beta * x))``

                  where alpha and beta are two multi-dimensional arrays of same size as x.
                - 'THRESHOLDEDRELU': Thresholded ReLU function, defined as:

                  ``f(x) = (x >= alpha) * x``

                  where alpha is a constant scalar.
                - 'LINEAR': linear function.

                   ``f(x) = alpha * x + beta``

        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        params: list of float or numpy.array
            Parameters for the activation, depending on non_linearity.

                - When non_linearity is one of ['RELU', 'SIGMOID', 'TANH', 'SCALED_TANH', 'SOFTPLUS', 'SOFTSIGN'], params is ignored.
                - When non_linearity is one of ['SCALED_TANH', 'SIGMOID_HARD', 'LINEAR'], param is a list of 2 floats
                  [alpha, beta].
                - When non_linearity is one of ['LEAKYRELU', 'ELU', 'THRESHOLDEDRELU'], param is a list of 1 float
                  [alpha].
                - When non_linearity is 'PRELU', param is a list of 1 numpy array [alpha]. The shape of
                  alpha is (C,), where C is either the number of input channels or
                  1. When C = 1, same alpha is applied to all channels.
                - When non_linearity is 'PARAMETRICSOFTPLUS', param is a list of 2 numpy arrays [alpha,
                  beta]. The shape of alpha and beta is (C, ), where C is either
                  the number of input channels or 1. When C = 1, same alpha and
                  beta are applied to all channels.

        See Also
        --------
        add_convolution, add_softmax
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.activation

        # Fill in the parameters
        non_linearity = non_linearity.upper() if isinstance(non_linearity, str) else non_linearity
        if non_linearity == 'RELU':
            spec_layer_params.ReLU.MergeFromString(b'')

        elif non_linearity == 'SIGMOID':
            spec_layer_params.sigmoid.MergeFromString(b'')

        elif non_linearity == 'TANH':
            spec_layer_params.tanh.MergeFromString(b'')

        elif non_linearity == 'SCALED_TANH':
            spec_layer_params.scaledTanh.MergeFromString(b'')
            if params is None:
                alpha, beta = (0.0, 0.0)
            else:
                alpha, beta = params[0], params[1]
            spec_layer_params.scaledTanh.alpha = alpha
            spec_layer_params.scaledTanh.beta = beta

        elif non_linearity == 'SOFTPLUS':
            spec_layer_params.softplus.MergeFromString(b'')

        elif non_linearity == 'SOFTSIGN':
            spec_layer_params.softsign.MergeFromString(b'')

        elif non_linearity == 'SIGMOID_HARD':
            if params is None:
                alpha, beta = (0.2, 0.5)
            else:
                alpha, beta = params[0], params[1]
            spec_layer_params.sigmoidHard.alpha = alpha
            spec_layer_params.sigmoidHard.beta = beta

        elif non_linearity == 'LEAKYRELU':
            if params is None:
                alpha = 0.3
            else:
                alpha = params[0]
            spec_layer_params.leakyReLU.alpha = float(alpha)

        elif non_linearity == 'PRELU':
            # PReLU must provide an np array in params[0]
            spec_layer_params.PReLU.alpha.floatValue.extend(map(float, params.flatten()))

        elif non_linearity == 'ELU':
            # ELU must provide an alpha in params[0]
            spec_layer_params.ELU.alpha = float(params)

        elif non_linearity == 'PARAMETRICSOFTPLUS':
            # Parametric softplus must provide two np arrays for alpha and beta
            alphas, betas = (params[0], params[1])
            # Weight alignment: Keras [H,W,C,F]
            spec_layer_params.parametricSoftplus.alpha.floatValue.extend(map(float, alphas.flatten()))
            spec_layer_params.parametricSoftplus.beta.floatValue.extend(map(float, betas.flatten()))

        elif non_linearity == 'THRESHOLDEDRELU':
            if params is None:
                theta = 1.0
            else:
                theta = params
            spec_layer_params.thresholdedReLU.alpha = float(theta)

        elif non_linearity == 'LINEAR':
            if params is None:
                alpha, beta = (1.0, 0.0)
            else:
                alpha, beta = params[0], params[1]
            spec_layer_params.linear.alpha = alpha
            spec_layer_params.linear.beta = beta
        else:
            raise TypeError("Unknown activation type %s." % non_linearity)
        return spec_layer

    def add_elementwise(self, name, input_names, output_name, mode, alpha=None):
        """
        Add an element-wise operation layer to the model.
        Refer to the specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            A list of input blob names of this layer. The input blobs should have the same shape.
        output_name: str
            The output blob name of this layer.
        mode: str
            A string specifying the mode of the elementwise layer. It can be one of the following:

            - 'CONCAT': concatenate input blobs along the channel axis.
            - 'SEQUENCE_CONCAT': concatenate input blobs along the sequence axis.
            - 'ADD': perform an element-wise summation over the input blobs.
            - 'MULTIPLY': perform an element-wise multiplication over the input blobs.
            - 'DOT': compute the dot product of the two input blobs. In this mode, the length of input_names should be 2.
            - 'COS': compute the cosine similarity of the two input blobs. In this mode, the length of input_names should be 2.
            - 'MAX': compute the element-wise maximum over the input blobs.
            - 'MIN': compute the element-wise minimum over the input blobs.
            - 'AVE': compute the element-wise average over the input blobs.

        alpha: float
            if mode == 'ADD' and there is only one input_name, alpha is added to the input
            if mode == 'MULTIPLY' and there is only one input_name, alpha is multiplied to the input

        See Also
        --------
        add_upsample, add_sequence_repeat

        """
        input_names = input_names if isinstance(input_names, list) else [input_names]
        spec_layer = self._add_generic_layer(name, input_names, [output_name])

        # add one of the following layers
        mode = mode.upper() if isinstance(mode, str) else mode
        if mode == 'CONCAT':
            spec_layer.concat.sequenceConcat = False
        elif mode == 'SEQUENCE_CONCAT':
            spec_layer.concat.sequenceConcat = True
        elif mode == 'ADD':
            spec_layer.add.MergeFromString(b'')
            if alpha:
                spec_layer.add.alpha = alpha
        elif mode == 'MULTIPLY':
            spec_layer.multiply.MergeFromString(b'')
            if alpha:
                spec_layer.multiply.alpha = alpha
        elif mode == 'COS':
            spec_layer.dot.cosineSimilarity = True
        elif mode == 'DOT':
            spec_layer.dot.cosineSimilarity = False
        elif mode == 'MAX':
            spec_layer.max.MergeFromString(b'')
        elif mode == 'MIN':
            spec_layer.min.MergeFromString(b'')
        elif mode == 'AVE':
            spec_layer.average.MergeFromString(b'')
        else:
            raise ValueError('Unsupported elementwise mode %s' % mode)
        return spec_layer

    def add_upsample(self, name, scaling_factor_h, scaling_factor_w, input_name, output_name, mode='NN'):
        """
        Add an upsample layer to the model.
        Refer to the **UpsampleLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        scaling_factor_h: int
            Scaling factor on the vertical direction.
        scaling_factor_w: int
            Scaling factor on the horizontal direction.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        mode: str
            Following values are supported:
            'NN': nearest neighbour
            'BILINEAR': bilinear interpolation

        See Also
        --------
        add_sequence_repeat, add_elementwise
        """
        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.upsample
        spec_layer_params.scalingFactor.append(scaling_factor_h)
        spec_layer_params.scalingFactor.append(scaling_factor_w)
        mode = mode.upper() if isinstance(mode, str) else mode
        if mode == 'NN':
            spec_layer_params.mode = _NeuralNetwork_pb2.UpsampleLayerParams.InterpolationMode.Value('NN')
        elif mode == 'BILINEAR':
            spec_layer_params.mode = _NeuralNetwork_pb2.UpsampleLayerParams.InterpolationMode.Value('BILINEAR')
        else:
            raise ValueError('Unsupported upsampling mode %s' % mode)
        return spec_layer

    def add_scale(self, name, W, b, has_bias, input_name, output_name, shape_scale=None, shape_bias=None):
        """
        Add a scale layer to the model.
        Refer to the **ScaleLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        W: int or numpy.array
            Scale of the input.
        b: int or numpy.array
            Bias to add to the input.
        has_bias: boolean
            Whether the bias vector of this layer is ignored in the spec.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        shape_scale: list of int or tuple of int
            List of ints that specifies the shape of the scale parameter. Can be [1] or [C] or [1,H,W] or [C,H,W].
        shape_bias: list of int
            List of ints that specifies the shape of the bias parameter (if present). Can be [1] or [C] or [1,H,W] or [C,H,W].

        See Also
        --------
        add_bias
        """

        if not shape_scale:
            shape_scale = [1]
        if not shape_bias:
            shape_bias = [1]

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.scale

        spec_layer_params.hasBias = has_bias

        # add scale and its shape
        scale = spec_layer_params.scale
        spec_layer_params.shapeScale.extend(shape_scale)
        if isinstance(W, int):
            scale.floatValue.append(float(W))
        else:
            scale.floatValue.extend(map(float, W.flatten()))
        if len(scale.floatValue) != np.prod(shape_scale):
            raise ValueError("Dimensions of 'shape_scale' do not match the size of the provided 'scale' parameter")

        # add bias and its shape
        if has_bias:
            bias = spec_layer_params.bias
            spec_layer_params.shapeBias.extend(shape_bias)
            if isinstance(b, int):
                bias.floatValue.append(float(b))
            else:
                bias.floatValue.extend(map(float, b.flatten()))
            if len(bias.floatValue) != np.prod(shape_bias):
                raise ValueError("Dimensions of 'shape_bias' do not match the size of the provided 'b' parameter")
        return spec_layer

    def add_bias(self, name, b, input_name, output_name, shape_bias=None):
        """
        Add a bias layer to the model.
        Refer to the **BiasLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        b: int or numpy.array
            Bias to add to the input.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        shape_bias: list of int
            List of ints that specifies the shape of the bias parameter (if present). Can be [1] or [C] or [1,H,W] or [C,H,W].

        See Also
        --------
        add_scale
        """

        if not shape_bias:
            shape_bias = [1]

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.bias

        # add bias and its shape
        bias = spec_layer_params.bias
        if len(shape_bias) != 1 and len(shape_bias) != 3:
            raise ValueError('Shape of bias layer must have length 1 or 3.')

        spec_layer_params.shape.extend(shape_bias)
        if isinstance(b, int):
            bias.floatValue.append(float(b))
        else:
            bias.floatValue.extend(map(float, b.flatten()))
        if len(bias.floatValue) != np.prod(shape_bias):
            raise ValueError("Dimensions of 'shape_bias' do not match the size"
                             "of the provided 'b' parameter")
        return spec_layer

    def add_sequence_repeat(self, name, nrep, input_name, output_name):
        """
        Add a sequence repeat layer to the model.
        Refer to the **SequenceRepeatLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        nrep: int
            Number of repetitions of the input blob along the sequence axis.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_upsample, add_elementwise
        """
        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.sequenceRepeat
        spec_layer_params.nRepetitions = nrep
        return spec_layer

    def add_convolution(self, name, kernel_channels, output_channels, height,
                        width, stride_height, stride_width, border_mode, groups, W, b, has_bias,
                        is_deconv=False, output_shape=None,
                        input_name='data', output_name='out',
                        dilation_factors=[1, 1],
                        padding_top=0, padding_bottom=0, padding_left=0, padding_right=0,
                        same_padding_asymmetry_mode='BOTTOM_RIGHT_HEAVY',
                        **kwargs):
        """
        Add a convolution layer to the network.
        Refer to the **ConvolutionLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        kernel_channels: int
            Number of channels for the convolution kernels.
        output_channels: int
            Number of filter kernels. This is equal to the number of channels in the output blob.
        height: int
            Height of each kernel.
        width: int
            Width of each kernel.
        stride_height: int
            Stride along the height direction.
        stride_width: int
            Stride along the height direction.
        border_mode: str
            Option for the padding type and output blob shape. Can be either 'valid' or 'same'.
        groups: int
            Number of kernel groups. Input is divided into groups along the channel axis. Each kernel group share the same weights.
        W: numpy.array or bytes() or None
            Weight of the convolution kernels.

            - If is_deconv is False, W should have shape (height, width, kernel_channels, output_channels), where kernel_channel = input_channels / groups
            - If is_deconv is True, W should have shape (height, width, kernel_channels, output_channels / groups), where kernel_channel = input_channels

            If W is of type bytes(), i.e. quantized, other quantization related arguments must be provided as well (see below).
            For Core ML specification version >=4, W can be None. In this case,
            the convolution layer takes 2 inputs, where the 1st input represents the input feature map,
            the 2nd input represents the weight blob.

        b: numpy.array
            Biases of the convolution kernels. b should have shape (outputChannels, ).
        has_bias: boolean
            Whether bias is ignored.

            - If True, bias is not ignored.
            - If False, bias is ignored.

        is_deconv: boolean
            Whether the convolution layer is performing a convolution or a transposed convolution (deconvolution).

            - If True, the convolution layer is performing transposed convolution.
            - If False, the convolution layer is performing regular convolution.

        output_shape: tuple or None
            Either None or a 2-tuple, specifying the output shape (output_height, output_width). Used only when is_deconv == True.
            When is_deconv == False, this parameter is ignored.
            If it is None, the output shape is calculated automatically using the border_mode.

        input_name: str or list of str
            The input blob name(s) of this layer.
        output_name: str
            The output blob name of this layer.

        dilation_factors: list of int
            Dilation factors across height and width directions. Must be a list of two positive integers.
            Defaults to [1, 1]

        padding_top, padding_bottom, padding_left, padding_right: int
            values of height (top, bottom) and width (left, right) padding to be used if border_more is "valid".

        same_padding_asymmetry_mode: str.
            Type of asymmetric padding to be used when  border_mode is 'same'.
            Can be either 'BOTTOM_RIGHT_HEAVY' or  'TOP_LEFT_HEAVY'.

        Depthwise convolution is a special case of convolution, where we have:
            kernel_channels = 1 (== input_channels / groups)
            output_channels = channel_multiplier * input_channels
            groups = input_channels
            W: [Kernel_height, Kernel_width, 1, channel_multiplier * input_channels]


        Quantization arguments expected in kwargs, when W is of type bytes():

            quantization_type: str
                When weights are quantized (i.e. W is of type bytes()), this should be either "linear" or "lut".

            nbits: int
                Should be between 1 and 8 (inclusive). Number of bits per weight value. Only applicable when
                weights are quantized.

            quant_scale: numpy.array(dtype=numpy.float32)
                scale vector to be used with linear quantization. Must be of length either 1 or output_channels.

            quant_bias: numpy.array(dtype=numpy.float32)
                bias vector to be used with linear quantization. Must be of length either 1 or output_channels.

            quant_lut: numpy.array(dtype=numpy.float32)
                the LUT (look up table) to be used with LUT quantization. Must be of length 2^nbits.

        See Also
        --------
        add_pooling, add_activation, add_batchnorm

        """

        if isinstance(input_name, tuple):
            input_names = list(input_name)
        elif isinstance(input_name, list):
            input_names = input_name
        else:
            input_names = [input_name]
        spec_layer = self._add_generic_layer(name, input_names, [output_name])

        # Set the layer params
        spec_layer_params = spec_layer.convolution
        spec_layer_params.isDeconvolution = is_deconv

        if is_deconv and output_shape:
            spec_layer_params.outputShape.append(output_shape[0])
            spec_layer_params.outputShape.append(output_shape[1])

        spec_layer_params.outputChannels = output_channels
        spec_layer_params.kernelChannels = kernel_channels
        spec_layer_params.kernelSize.append(height)
        spec_layer_params.kernelSize.append(width)
        spec_layer_params.stride.append(stride_height)
        spec_layer_params.stride.append(stride_width)

        border_mode = border_mode.lower() if isinstance(border_mode, str) else border_mode
        same_padding_asymmetry_mode = same_padding_asymmetry_mode.upper() \
            if isinstance(same_padding_asymmetry_mode, str) else same_padding_asymmetry_mode

        if border_mode == 'valid':
            height_border = spec_layer_params.valid.paddingAmounts.borderAmounts.add()
            height_border.startEdgeSize = padding_top
            height_border.endEdgeSize = padding_bottom
            width_border = spec_layer_params.valid.paddingAmounts.borderAmounts.add()
            width_border.startEdgeSize = padding_left
            width_border.endEdgeSize = padding_right
        elif border_mode == 'same':
            if not (
                    same_padding_asymmetry_mode == 'BOTTOM_RIGHT_HEAVY' or same_padding_asymmetry_mode == 'TOP_LEFT_HEAVY'):
                raise ValueError(
                    "Invalid value %d of same_padding_asymmetry_mode parameter" % same_padding_asymmetry_mode)
            spec_layer_params.same.asymmetryMode = _NeuralNetwork_pb2.SamePadding.SamePaddingMode.Value(
                same_padding_asymmetry_mode)
        else:
            raise NotImplementedError(
                'Border mode %s is not implemented.' % border_mode)

        spec_layer_params.nGroups = groups
        spec_layer_params.hasBias = has_bias

        # add dilation factors
        spec_layer_params.dilationFactor.append(dilation_factors[0])
        spec_layer_params.dilationFactor.append(dilation_factors[1])

        # If weight comes from another tensor just return
        if len(input_names) > 1:
            return

        # Weight assignments
        if len(kwargs) > 0:
            _verify_quantization_arguments(weight=W, output_channels=output_channels, **kwargs)

            nbits = kwargs.get('nbits', 8)
            num_weights = (output_channels * kernel_channels * height * width) / groups
            if nbits < 8:
                byte_arr = np.frombuffer(W, dtype=np.uint8)
                W = unpack_to_bytes(byte_arr, num_weights, nbits)
            else:
                W = np.frombuffer(W, dtype=np.uint8)

            if is_deconv:
                W = np.reshape(W, (height, width, kernel_channels, output_channels / groups))
            else:
                W = np.reshape(W, (height, width, kernel_channels, output_channels))

        # Weight alignment: MLModel Spec requires following weight arrangement:
        # is_deconv == False ==> (output_channels, kernel_channels, height, width), where kernel_channel = input_channels / groups
        # is_deconv == True ==> (kernel_channels, output_channels / groups, height, width), where kernel_channel = input_channels
        if not is_deconv:
            Wt = W.transpose((3, 2, 0, 1))
            Wt = Wt.flatten()
        else:
            Wt = W.transpose((2, 3, 0, 1)).flatten()

        # Assign weights
        weights = spec_layer_params.weights
        if len(kwargs) == 0:  # no quantization
            weights.floatValue.extend(map(float, Wt.flatten()))
        else:  # there is quantization
            W_bytes = bytes()
            if nbits == 8:
                W_bytes += Wt.flatten().tobytes()
            else:
                W_bytes += _convert_array_to_nbit_quantized_bytes(Wt.flatten(), nbits).tobytes()
            _fill_quantized_weights(weights_message=weights, W=W_bytes, **kwargs)

        # Assign biases
        if has_bias:
            bias = spec_layer_params.bias
            for f in range(output_channels):
                bias.floatValue.append(float(b[f]))

        return spec_layer

    def add_pooling(self, name, height, width, stride_height, stride_width,
                    layer_type, padding_type, input_name, output_name, exclude_pad_area=True, is_global=False,
                    padding_top=0, padding_bottom=0, padding_left=0, padding_right=0,
                    same_padding_asymmetry_mode='BOTTOM_RIGHT_HEAVY'):
        """
        Add a pooling layer to the model that performs spatial pooling.
        Refer to the **PoolingLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        height: int
            Height of pooling region.
        width: int
            Width of pooling region.
        stride_height: int
            Stride along the height direction.
        stride_width: int
            Stride along the width direction.
        layer_type: str
            Type of pooling performed. Can either be 'MAX', 'AVERAGE' or 'L2'.
        padding_type: str
            Option for the type of padding and output blob shape. Can be either 'VALID' , 'SAME' or 'INCLUDE_LAST_PIXEL'.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        exclude_pad_area: boolean
            Whether to exclude padded area in the 'AVERAGE' pooling operation, default: true.
            - If True, the value of the padded area will be excluded.
            - If False, the padded area will be included.
            This flag is only used with average pooling.
        is_global: boolean
            Whether the pooling operation is global. Defaults to False.
            - If True, the pooling operation is global -- the pooling region is of the same size of the input blob.
            Parameters height, width, stride_height, stride_width will be ignored.
            - If False, the pooling operation is not global.
        padding_top, padding_bottom, padding_left, padding_right: int
            values of height (top, bottom) and width (left, right) padding to be used if padding type is "VALID" or "INCLUDE_LAST_PIXEL".
        same_padding_asymmetry_mode: str.
            Type of asymmetric padding to be used when padding_type = 'SAME'.
            Can be either 'BOTTOM_RIGHT_HEAVY' or 'TOP_LEFT_HEAVY'.

        See Also
        --------
        add_convolution, add_activation
        """
        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.pooling

        # Set the parameters
        spec_layer_params.type = \
            _NeuralNetwork_pb2.PoolingLayerParams.PoolingType.Value(layer_type.upper())

        padding_type = padding_type.upper() if isinstance(padding_type, str) else padding_type
        same_padding_asymmetry_mode = same_padding_asymmetry_mode.upper() \
            if isinstance(same_padding_asymmetry_mode, str) else same_padding_asymmetry_mode

        if padding_type == 'VALID':
            height_border = spec_layer_params.valid.paddingAmounts.borderAmounts.add()
            height_border.startEdgeSize = padding_top
            height_border.endEdgeSize = padding_bottom
            width_border = spec_layer_params.valid.paddingAmounts.borderAmounts.add()
            width_border.startEdgeSize = padding_left
            width_border.endEdgeSize = padding_right
        elif padding_type == 'SAME':
            if not (
                    same_padding_asymmetry_mode == 'BOTTOM_RIGHT_HEAVY' or same_padding_asymmetry_mode == 'TOP_LEFT_HEAVY'):
                raise ValueError(
                    "Invalid value %d of same_padding_asymmetry_mode parameter" % same_padding_asymmetry_mode)
            spec_layer_params.same.asymmetryMode = _NeuralNetwork_pb2.SamePadding.SamePaddingMode.Value(
                same_padding_asymmetry_mode)
        elif padding_type == 'INCLUDE_LAST_PIXEL':
            if padding_top != padding_bottom or padding_left != padding_right:
                raise ValueError("Only symmetric padding is supported with the INCLUDE_LAST_PIXEL padding type")
            spec_layer_params.includeLastPixel.paddingAmounts.append(padding_top)
            spec_layer_params.includeLastPixel.paddingAmounts.append(padding_left)
        else:
            raise ValueError("Unknown padding_type %s in pooling" % padding_type)

        spec_layer_params.kernelSize.append(height)
        spec_layer_params.kernelSize.append(width)
        spec_layer_params.stride.append(stride_height)
        spec_layer_params.stride.append(stride_width)
        spec_layer_params.avgPoolExcludePadding = exclude_pad_area
        spec_layer_params.globalPooling = is_global
        return spec_layer

    def add_padding(self, name, left=0, right=0, top=0, bottom=0,
                    value=0, input_name='data', output_name='out',
                    padding_type='constant'):
        """
        Add a padding layer to the model that performs padding along spatial dimensions.
        Refer to the **PaddingLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        left: int
            Number of elements to be padded on the left side of the input blob.
        right: int
            Number of elements to be padded on the right side of the input blob.
        top: int
            Number of elements to be padded on the top of the input blob.
        bottom: int
            Number of elements to be padded on the bottom of the input blob.
        value: float
            Value of the elements padded. Used only when padding_type = 'constant'
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        padding_type: str
            Type of the padding. Can be one of 'constant', 'reflection' or 'replication'.

        See Also
        --------
        add_crop, add_convolution, add_pooling, add_constant_pad
        """
        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.padding

        # Set the parameters
        padding_type = padding_type.lower() if isinstance(padding_type, str) else padding_type
        if padding_type == 'constant':
            spec_layer_params.constant.value = value
        elif padding_type == 'reflection':
            spec_layer_params.reflection.MergeFromString(b'')
        elif padding_type == 'replication':
            spec_layer_params.replication.MergeFromString(b'')
        else:
            raise ValueError("Unknown padding_type %s" % padding_type)

        height_border = spec_layer_params.paddingAmounts.borderAmounts.add()
        height_border.startEdgeSize = top
        height_border.endEdgeSize = bottom
        width_border = spec_layer_params.paddingAmounts.borderAmounts.add()
        width_border.startEdgeSize = left
        width_border.endEdgeSize = right
        return spec_layer

    def add_crop(self, name, left, right, top, bottom, offset, input_names,
                 output_name):
        """
        Add a cropping layer to the model.
        The cropping layer have two functional modes:

            - When it has 1 input blob, it crops the input blob based
              on the 4 parameters [left, right, top, bottom].
            - When it has 2 input blobs, it crops the first input blob based
              on the dimension of the second blob with an offset.

        Refer to the **CropLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        left: int
            Number of elements to be cropped on the left side of the input blob.
            When the crop layer takes 2 inputs, this parameter is ignored.
        right: int
            Number of elements to be cropped on the right side of the input blob.
            When the crop layer takes 2 inputs, this parameter is ignored.
        top: int
            Number of elements to be cropped on the top of the input blob.
            When the crop layer takes 2 inputs, this parameter is ignored.
        bottom: int
            Number of elements to be cropped on the bottom of the input blob.
            When the crop layer takes 2 inputs, this parameter is ignored.
        offset: list of int
            Offset along the height and width directions when the crop layer takes 2 inputs. Must be a list of length 2.
            When the crop layer takes 1 input, this parameter is ignored.
        input_names: list of str
            The input blob names of this layer. Must be either a list of 1 string (1 input crop layer),
            or a list of 2 strings (2-input crop layer).
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_padding, add_convolution, add_pooling
        """
        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer_params = spec_layer.crop

        # Set the parameters
        offset = [0, 0] if len(input_names) == 1 else offset
        spec_layer_params.offset.extend(offset)
        height_border = spec_layer_params.cropAmounts.borderAmounts.add()
        height_border.startEdgeSize = top
        height_border.endEdgeSize = bottom
        width_border = spec_layer_params.cropAmounts.borderAmounts.add()
        width_border.startEdgeSize = left
        width_border.endEdgeSize = right
        return spec_layer

    def add_simple_rnn(self, name, W_h, W_x, b, hidden_size, input_size, activation, input_names, output_names,
                       output_all=False, reverse_input=False):
        """
        Add a simple recurrent layer to the model.
        Refer to the **SimpleRecurrentLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        W_h: numpy.array
            Weights of the recurrent layer's hidden state. Must be of shape (hidden_size, hidden_size).
        W_x: numpy.array
            Weights of the recurrent layer's input. Must be of shape (hidden_size, input_size).
        b: numpy.array or None
            Bias of the recurrent layer's output. If None, bias is ignored. Otherwise it must be of shape (hidden_size, ).
        hidden_size: int
            Number of hidden units. This is equal to the number of channels of output shape.
        input_size: int
            Number of the number of channels of input shape.
        activation: str
            Activation function name. Can be one of the following option:
            ['RELU', 'TANH', 'SIGMOID', 'SCALED_TANH', 'SIGMOID_HARD', 'LINEAR'].
            See add_activation for more detailed description.
        input_names: list of str
            The input blob names list of this layer, in the order of [x, h_input].
        output_names: list of str
            The output blob names list of this layer, in the order of [y, h_output].
        output_all: boolean
            Whether the recurrent layer should output at every time step.

            - If False, the output is the result after the final state update.
            - If True, the output is a sequence, containing outputs at all time steps.
        reverse_input: boolean
            Whether the recurrent layer should process the input sequence in the reverse order.

            - If False, the input sequence order is not reversed.
            - If True, the input sequence order is reversed.

        See Also
        --------
        add_activation, add_gru, add_unilstm, add_bidirlstm
        """
        spec_layer = self._add_generic_layer(name, input_names, output_names)
        spec_layer_params = spec_layer.simpleRecurrent
        spec_layer_params.reverseInput = reverse_input

        # set the parameters
        spec_layer_params.inputVectorSize = input_size
        spec_layer_params.outputVectorSize = hidden_size
        if b is not None:
            spec_layer_params.hasBiasVector = True
        spec_layer_params.sequenceOutput = output_all

        activation_f = spec_layer_params.activation
        _set_recurrent_activation(activation_f, activation)

        # Write the weights
        spec_layer_params.weightMatrix.floatValue.extend(map(float, W_x.flatten()))
        spec_layer_params.recursionMatrix.floatValue.extend(map(float, W_h.flatten()))

        if b is not None:
            spec_layer_params.biasVector.floatValue.extend(map(float, b.flatten()))
        return spec_layer

    def add_gru(self, name, W_h, W_x, b, hidden_size, input_size,
                input_names, output_names, activation='TANH', inner_activation='SIGMOID_HARD',
                output_all=False, reverse_input=False):
        """
        Add a Gated-Recurrent Unit (GRU) layer to the model.
        Refer to the **GRULayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        W_h: [numpy.array]
            List of recursion weight matrices. The ordering is [R_z, R_r, R_o],
            where R_z, R_r and R_o are weight matrices at update gate, reset gate and output gate.
            The shapes of these matrices are (hidden_size, hidden_size).
        W_x: [numpy.array]
            List of input weight matrices. The ordering is [W_z, W_r, W_o],
            where W_z, W_r, and W_o are weight matrices at update gate, reset gate and output gate.
            The shapes of these matrices are (hidden_size, input_size).
        b: [numpy.array] or None
            List of biases of the GRU layer. The ordering is [b_z, b_r, b_o],
            where b_z, b_r, b_o are biases at update gate, reset gate and output gate.
            If None, biases are ignored. Otherwise the shapes of the biases are (hidden_size, ).
        hidden_size: int
            Number of hidden units. This is equal to the number of channels of output shape.
        input_size: int
            Number of the number of channels of input shape.
        activation: str
            Activation function used at the output gate. Can be one of the following option:
            ['RELU', 'TANH', 'SIGMOID', 'SCALED_TANH', 'SIGMOID_HARD', 'LINEAR'].
            Defaults to 'TANH'.
            See add_activation for more detailed description.
        inner_activation: str
            Inner activation function used at update and reset gates. Can be one of the following option:
            ['RELU', 'TANH', 'SIGMOID', 'SCALED_TANH', 'SIGMOID_HARD', 'LINEAR'].
            Defaults to 'SIGMOID_HARD'.
            See add_activation for more detailed description.
        input_names: list of str
            The input blob names list of this layer, in the order of [x, h_input].
        output_names: list of str
            The output blob names list of this layer, in the order of [y, h_output].
        output_all: boolean
            Whether the recurrent layer should output at every time step.

            - If False, the output is the result after the final state update.
            - If True, the output is a sequence, containing outputs at all time steps.
        reverse_input: boolean
            Whether the recurrent layer should process the input sequence in the reverse order.

            - If False, the input sequence order is not reversed.
            - If True, the input sequence order is reversed.

        See Also
        --------
        add_activation, add_simple_rnn, add_unilstm, add_bidirlstm
        """
        spec_layer = self._add_generic_layer(name, input_names, output_names)
        spec_layer_params = spec_layer.gru

        # set the parameters
        spec_layer_params.inputVectorSize = input_size
        spec_layer_params.outputVectorSize = hidden_size
        if b is not None:
            spec_layer_params.hasBiasVectors = True
        spec_layer_params.sequenceOutput = output_all
        spec_layer_params.reverseInput = reverse_input

        activation_f = spec_layer_params.activations.add()
        activation_g = spec_layer_params.activations.add()
        _set_recurrent_activation(activation_f, inner_activation)
        _set_recurrent_activation(activation_g, activation)

        # Write the weights
        R_z, R_r, R_o = W_h
        W_z, W_r, W_o = W_x

        spec_layer_params.updateGateWeightMatrix.floatValue.extend(map(float, W_z.flatten()))
        spec_layer_params.resetGateWeightMatrix.floatValue.extend(map(float, W_r.flatten()))
        spec_layer_params.outputGateWeightMatrix.floatValue.extend(map(float, W_o.flatten()))

        spec_layer_params.updateGateRecursionMatrix.floatValue.extend(map(float, R_z.flatten()))
        spec_layer_params.resetGateRecursionMatrix.floatValue.extend(map(float, R_r.flatten()))
        spec_layer_params.outputGateRecursionMatrix.floatValue.extend(map(float, R_o.flatten()))

        if b is not None:
            b_z, b_r, b_o = b
            spec_layer_params.updateGateBiasVector.floatValue.extend(map(float, b_z.flatten()))
            spec_layer_params.resetGateBiasVector.floatValue.extend(map(float, b_r.flatten()))
            spec_layer_params.outputGateBiasVector.floatValue.extend(map(float, b_o.flatten()))
        return spec_layer

    def add_unilstm(self, name, W_h, W_x, b, hidden_size, input_size, input_names, output_names,
                    inner_activation='SIGMOID',
                    cell_state_update_activation='TANH',
                    output_activation='TANH',
                    peep=None,
                    output_all=False,
                    forget_bias=False, coupled_input_forget_gate=False,
                    cell_clip_threshold=50000.0, reverse_input=False):
        """
        Add a Uni-directional LSTM layer to the model.
        Refer to the **UniDirectionalLSTMLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        W_h: [numpy.array]
            List of recursion weight matrices. The ordering is [R_i, R_f, R_o, R_z],
            where R_i, R_f, R_o, R_z are weight matrices at input gate, forget gate, output gate and cell gate.
            The shapes of these matrices are (hidden_size, hidden_size).
        W_x: [numpy.array]
            List of input weight matrices. The ordering is [W_i, W_f, W_o, W_z],
            where W_i, W_f, W_o, W_z are weight matrices at input gate, forget gate, output gate and cell gate.
            The shapes of these matrices are (hidden_size, input_size).
        b: [numpy.array] or None
            List of biases. The ordering is [b_i, b_f, b_o, b_z],
            where b_i, b_f, b_o, b_z are biases at input gate, forget gate, output gate and cell gate.
            If None, biases are ignored. Otherwise the shapes of the biases are (hidden_size, ).
        hidden_size: int
            Number of hidden units. This is equal to the number of channels of output shape.
        input_size: int
            Number of the number of channels of input shape.
        input_names: list of str
            The input blob names list of this layer, in the order of [x, h_input, c_input].
        output_names: list of str
            The output blob names list of this layer, in the order of [y, h_output, c_output].
        inner_activation: str
            Inner activation function used at input and forget gate. Can be one of the following option:
            ['RELU', 'TANH', 'SIGMOID', 'SCALED_TANH', 'SIGMOID_HARD', 'LINEAR'].
        cell_state_update_activation: str
            Cell state update activation function used at the cell state update gate.
            ['RELU', 'TANH', 'SIGMOID', 'SCALED_TANH', 'SIGMOID_HARD', 'LINEAR'].
        output_activation: str
            Activation function used at the output gate. Can be one of the following option:
            ['RELU', 'TANH', 'SIGMOID', 'SCALED_TANH', 'SIGMOID_HARD', 'LINEAR'].
        peep: [numpy.array] or None
            List of peephole vectors. The ordering is [p_i, p_f, p_o],
            where p_i, p_f, and p_o are peephole vectors at input gate, forget gate, output gate.
            The shapes of the peephole vectors are (hidden_size,).
        output_all: boolean
            Whether the LSTM layer should output at every time step.

            - If False, the output is the result after the final state update.
            - If True, the output is a sequence, containing outputs at all time steps.

        forget_bias: boolean
            If True, a vector of 1s is added to forget gate bias.
        coupled_input_forget_gate: boolean
            If True, the input gate and forget gate is coupled. i.e. forget gate is not used.
        cell_clip_threshold: float
            The limit on the maximum and minimum values on the cell state.
            If not provided, it is defaulted to 50.0.
        reverse_input: boolean
            Whether the LSTM layer should process the input sequence in the reverse order.

            - If False, the input sequence order is not reversed.
            - If True, the input sequence order is reversed.

        See Also
        --------
        add_activation, add_simple_rnn, add_gru, add_bidirlstm
        """
        spec_layer = self._add_generic_layer(name, input_names, output_names)
        spec_layer_params = spec_layer.uniDirectionalLSTM
        params = spec_layer_params.params
        weight_params = spec_layer_params.weightParams

        # set the parameters
        spec_layer_params.inputVectorSize = input_size
        spec_layer_params.outputVectorSize = hidden_size
        params.sequenceOutput = output_all
        params.forgetBias = False
        if b is not None:
            params.hasBiasVectors = True
        if peep is not None:
            params.hasPeepholeVectors = True
        params.coupledInputAndForgetGate = coupled_input_forget_gate
        params.cellClipThreshold = cell_clip_threshold
        params.forgetBias = forget_bias

        spec_layer_params.reverseInput = reverse_input

        activation_f = spec_layer_params.activations.add()
        activation_g = spec_layer_params.activations.add()
        activation_h = spec_layer_params.activations.add()
        _set_recurrent_activation(activation_f, inner_activation)
        _set_recurrent_activation(activation_g, cell_state_update_activation)
        _set_recurrent_activation(activation_h, output_activation)

        # Write the weights
        R_i, R_f, R_o, R_z = W_h
        W_i, W_f, W_o, W_z = W_x

        weight_params.inputGateWeightMatrix.floatValue.extend(map(float, W_i.flatten()))
        weight_params.forgetGateWeightMatrix.floatValue.extend(map(float, W_f.flatten()))
        weight_params.outputGateWeightMatrix.floatValue.extend(map(float, W_o.flatten()))
        weight_params.blockInputWeightMatrix.floatValue.extend(map(float, W_z.flatten()))

        weight_params.inputGateRecursionMatrix.floatValue.extend(map(float, R_i.flatten()))
        weight_params.forgetGateRecursionMatrix.floatValue.extend(map(float, R_f.flatten()))
        weight_params.outputGateRecursionMatrix.floatValue.extend(map(float, R_o.flatten()))
        weight_params.blockInputRecursionMatrix.floatValue.extend(map(float, R_z.flatten()))

        if b is not None:
            b_i, b_f, b_o, b_z = b
            weight_params.inputGateBiasVector.floatValue.extend(map(float, b_i.flatten()))
            weight_params.forgetGateBiasVector.floatValue.extend(map(float, b_f.flatten()))
            weight_params.outputGateBiasVector.floatValue.extend(map(float, b_o.flatten()))
            weight_params.blockInputBiasVector.floatValue.extend(map(float, b_z.flatten()))

        if peep is not None:
            p_i, p_f, p_o = peep
            weight_params.inputGatePeepholeVector.floatValue.extend(map(float, p_i.flatten()))
            weight_params.forgetGatePeepholeVector.floatValue.extend(map(float, p_f.flatten()))
            weight_params.outputGatePeepholeVector.floatValue.extend(map(float, p_o.flatten()))

        return spec_layer

    def add_bidirlstm(self, name, W_h, W_x, b, W_h_back, W_x_back, b_back, hidden_size, input_size,
                      input_names, output_names,
                      inner_activation='SIGMOID',
                      cell_state_update_activation='TANH',
                      output_activation='TANH',
                      peep=None, peep_back=None,
                      output_all=False,
                      forget_bias=False, coupled_input_forget_gate=False, cell_clip_threshold=50000.0):

        """
        Add a Bi-directional LSTM layer to the model.
        Refer to the **BiDirectionalLSTMLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        W_h: [numpy.array]
            List of recursion weight matrices for the forward layer. The ordering is [R_i, R_f, R_o, R_z],
            where R_i, R_f, R_o, R_z are weight matrices at input gate, forget gate, output gate and cell gate.
            The shapes of these matrices are (hidden_size, hidden_size).
        W_x: [numpy.array]
            List of input weight matrices for the forward layer. The ordering is [W_i, W_f, W_o, W_z],
            where W_i, W_f, W_o, W_z are weight matrices at input gate, forget gate, output gate and cell gate.
            The shapes of these matrices are (hidden_size, input_size).
        b: [numpy.array]
            List of biases for the forward layer. The ordering is [b_i, b_f, b_o, b_z],
            where b_i, b_f, b_o, b_z are biases at input gate, forget gate, output gate and cell gate.
            If None, biases are ignored. Otherwise the shapes of the biases are (hidden_size, ).
        W_h_back: [numpy.array]
            List of recursion weight matrices for the backward layer. The ordering is [R_i, R_f, R_o, R_z],
            where R_i, R_f, R_o, R_z are weight matrices at input gate, forget gate, output gate and cell gate.
            The shapes of these matrices are (hidden_size, hidden_size).
        W_x_back: [numpy.array]
            List of input weight matrices for the backward layer. The ordering is [W_i, W_f, W_o, W_z],
            where W_i, W_f, W_o, W_z are weight matrices at input gate, forget gate, output gate and cell gate.
            The shapes of these matrices are (hidden_size, input_size).
        b_back: [numpy.array]
            List of biases for the backward layer. The ordering is [b_i, b_f, b_o, b_z],
            where b_i, b_f, b_o, b_z are biases at input gate, forget gate, output gate and cell gate.
            The shapes of the biases (hidden_size).
        hidden_size: int
            Number of hidden units. This is equal to the number of channels of output shape.
        input_size: int
            Number of the number of channels of input shape.
        input_names: list of str
            The input blob names of this layer, in the order of [x, h_input, c_input, h_reverse_input, c_reverse_input].
        output_names: list of str
            The output blob names of this layer, in the order of [y, h_output, c_output, h_reverse_output, c_reverse_output].
        inner_activation: str
            Inner activation function used at input and forget gate. Can be one of the following option:
            ['RELU', 'TANH', 'SIGMOID', 'SCALED_TANH', 'SIGMOID_HARD', 'LINEAR'].
            Defaults to 'SIGMOID'.
        cell_state_update_activation: str
            Cell state update activation function used at the cell state update gate.
            ['RELU', 'TANH', 'SIGMOID', 'SCALED_TANH', 'SIGMOID_HARD', 'LINEAR'].
            Defaults to 'TANH'.
        output_activation: str
            Activation function used at the output gate. Can be one of the following option:
            ['RELU', 'TANH', 'SIGMOID', 'SCALED_TANH', 'SIGMOID_HARD', 'LINEAR'].
            Defaults to 'TANH'.
        peep: [numpy.array] or None
            List of peephole vectors for the forward layer. The ordering is [p_i, p_f, p_o],
            where p_i, p_f, and p_o are peephole vectors at input gate, forget gate, output gate.
            The shapes of the peephole vectors are (hidden_size,). Defaults to None.
        peep_back: [numpy.array] or None
            List of peephole vectors for the backward layer. The ordering is [p_i, p_f, p_o],
            where p_i, p_f, and p_o are peephole vectors at input gate, forget gate, output gate.
            The shapes of the peephole vectors are (hidden_size,). Defaults to None.
        output_all: boolean
            Whether the LSTM layer should output at every time step. Defaults to False.

            - If False, the output is the result after the final state update.
            - If True, the output is a sequence, containing outputs at all time steps.

        forget_bias: boolean
            If True, a vector of 1s is added to forget gate bias. Defaults to False.
        coupled_input_forget_gate: boolean
            If True, the input gate and forget gate is coupled. i.e. forget gate is not used.
            Defaults to False.
        cell_clip_threshold: float
            The limit on the maximum and minimum values on the cell state.
            Defaults to 50.0.

        See Also
        --------
        add_activation, add_simple_rnn, add_unilstm, add_bidirlstm
        """
        spec_layer = self._add_generic_layer(name, input_names, output_names)
        spec_layer_params = spec_layer.biDirectionalLSTM
        params = spec_layer_params.params
        weight_params = spec_layer_params.weightParams.add()
        weight_params_back = spec_layer_params.weightParams.add()

        # set the parameters
        spec_layer_params.inputVectorSize = input_size
        spec_layer_params.outputVectorSize = hidden_size
        if b is not None:
            params.hasBiasVectors = True
        params.sequenceOutput = output_all
        params.forgetBias = forget_bias
        if peep is not None:
            params.hasPeepholeVectors = True
        params.coupledInputAndForgetGate = coupled_input_forget_gate
        params.cellClipThreshold = cell_clip_threshold

        # set activations
        activation_f = spec_layer_params.activationsForwardLSTM.add()
        activation_g = spec_layer_params.activationsForwardLSTM.add()
        activation_h = spec_layer_params.activationsForwardLSTM.add()
        _set_recurrent_activation(activation_f, inner_activation)
        _set_recurrent_activation(activation_g, cell_state_update_activation)
        _set_recurrent_activation(activation_h, output_activation)

        activation_f_back = spec_layer_params.activationsBackwardLSTM.add()
        activation_g_back = spec_layer_params.activationsBackwardLSTM.add()
        activation_h_back = spec_layer_params.activationsBackwardLSTM.add()
        _set_recurrent_activation(activation_f_back, inner_activation)
        _set_recurrent_activation(activation_g_back, cell_state_update_activation)
        _set_recurrent_activation(activation_h_back, output_activation)

        # Write the forward lstm weights
        R_i, R_f, R_o, R_z = W_h
        W_i, W_f, W_o, W_z = W_x

        weight_params.inputGateWeightMatrix.floatValue.extend(map(float, W_i.flatten()))
        weight_params.forgetGateWeightMatrix.floatValue.extend(map(float, W_f.flatten()))
        weight_params.outputGateWeightMatrix.floatValue.extend(map(float, W_o.flatten()))
        weight_params.blockInputWeightMatrix.floatValue.extend(map(float, W_z.flatten()))

        weight_params.inputGateRecursionMatrix.floatValue.extend(map(float, R_i.flatten()))
        weight_params.forgetGateRecursionMatrix.floatValue.extend(map(float, R_f.flatten()))
        weight_params.outputGateRecursionMatrix.floatValue.extend(map(float, R_o.flatten()))
        weight_params.blockInputRecursionMatrix.floatValue.extend(map(float, R_z.flatten()))

        if b is not None:
            b_i, b_f, b_o, b_z = b
            weight_params.inputGateBiasVector.floatValue.extend(map(float, b_i.flatten()))
            weight_params.forgetGateBiasVector.floatValue.extend(map(float, b_f.flatten()))
            weight_params.outputGateBiasVector.floatValue.extend(map(float, b_o.flatten()))
            weight_params.blockInputBiasVector.floatValue.extend(map(float, b_z.flatten()))

        if peep is not None:
            p_i, p_f, p_o = peep
            weight_params.inputGatePeepholeVector.floatValue.extend(map(float, p_i.flatten()))
            weight_params.forgetGatePeepholeVector.floatValue.extend(map(float, p_f.flatten()))
            weight_params.outputGatePeepholeVector.floatValue.extend(map(float, p_o.flatten()))

        # Write the backward lstm weights
        R_i, R_f, R_o, R_z = W_h_back
        W_i, W_f, W_o, W_z = W_x_back

        weight_params_back.inputGateWeightMatrix.floatValue.extend(map(float, W_i.flatten()))
        weight_params_back.forgetGateWeightMatrix.floatValue.extend(map(float, W_f.flatten()))
        weight_params_back.outputGateWeightMatrix.floatValue.extend(map(float, W_o.flatten()))
        weight_params_back.blockInputWeightMatrix.floatValue.extend(map(float, W_z.flatten()))

        weight_params_back.inputGateRecursionMatrix.floatValue.extend(map(float, R_i.flatten()))
        weight_params_back.forgetGateRecursionMatrix.floatValue.extend(map(float, R_f.flatten()))
        weight_params_back.outputGateRecursionMatrix.floatValue.extend(map(float, R_o.flatten()))
        weight_params_back.blockInputRecursionMatrix.floatValue.extend(map(float, R_z.flatten()))

        if b_back is not None:
            b_i, b_f, b_o, b_z = b_back
            weight_params_back.inputGateBiasVector.floatValue.extend(map(float, b_i.flatten()))
            weight_params_back.forgetGateBiasVector.floatValue.extend(map(float, b_f.flatten()))
            weight_params_back.outputGateBiasVector.floatValue.extend(map(float, b_o.flatten()))
            weight_params_back.blockInputBiasVector.floatValue.extend(map(float, b_z.flatten()))

        if peep_back is not None:
            p_i, p_f, p_o = peep_back
            weight_params_back.inputGatePeepholeVector.floatValue.extend(map(float, p_i.flatten()))
            weight_params_back.forgetGatePeepholeVector.floatValue.extend(map(float, p_f.flatten()))
            weight_params_back.outputGatePeepholeVector.floatValue.extend(map(float, p_o.flatten()))
        return spec_layer

    def add_flatten(self, name, mode, input_name, output_name):
        """
        Add a flatten layer. Only flattens the channel, height and width axis. Leaves the sequence axis as is.
        Refer to the **FlattenLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        mode: int

            - If mode == 0, the flatten layer is in CHANNEL_FIRST mode.
            - If mode == 1, the flatten layer is in CHANNEL_LAST mode.

        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_permute, add_reshape
        """
        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.flatten

        # Set the parameters
        if mode == 0:
            spec_layer_params.mode = \
                _NeuralNetwork_pb2.FlattenLayerParams.FlattenOrder.Value('CHANNEL_FIRST')
        elif mode == 1:
            spec_layer_params.mode = \
                _NeuralNetwork_pb2.FlattenLayerParams.FlattenOrder.Value('CHANNEL_LAST')
        else:
            raise NotImplementedError('Unknown flatten mode %d ' % mode)

        return spec_layer

    def add_slice(self, name, input_name, output_name, axis, start_index=0, end_index=-1, stride=1):
        """
        Add a slice layer. Equivalent to to numpy slice [start_index:end_index:stride],
        start_index is included, while end_index is exclusive.
        Refer to the **SliceLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.

        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        axis: str
            axis along which input is sliced.
            allowed values: 'channel', 'height', 'width'
        start_index: int
            must be non-negative.
        end_index: int
            negative indexing is supported.
        stride: int
            must be positive.

        See Also
        --------
        add_permute, add_reshape
        """
        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.slice

        # Set the parameters
        if start_index < 0:
            raise ValueError("Invalid start_index value %d. Must be non-negative." % start_index)
        if stride < 1:
            raise ValueError("Invalid stride value %d. Must be positive." % stride)

        spec_layer_params.startIndex = start_index
        spec_layer_params.endIndex = end_index
        spec_layer_params.stride = stride

        axis = axis.lower() if isinstance(axis, str) else axis
        if axis == 'channel':
            spec_layer_params.axis = \
                _NeuralNetwork_pb2.SliceLayerParams.SliceAxis.Value('CHANNEL_AXIS')
        elif axis == 'height':
            spec_layer_params.axis = \
                _NeuralNetwork_pb2.SliceLayerParams.SliceAxis.Value('HEIGHT_AXIS')
        elif axis == 'width':
            spec_layer_params.axis = \
                _NeuralNetwork_pb2.SliceLayerParams.SliceAxis.Value('WIDTH_AXIS')
        else:
            raise NotImplementedError('Unsupported Slice axis %s ' % axis)
        return spec_layer

    def add_reorganize_data(self, name, input_name, output_name, mode='SPACE_TO_DEPTH', block_size=2):
        """
        Add a data reorganization layer of type "SPACE_TO_DEPTH" or "DEPTH_TO_SPACE".
        Refer to the specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.

        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        mode: str

            - If mode == 'SPACE_TO_DEPTH': data is moved from the spatial to the channel dimension.
              Input is spatially divided into non-overlapping blocks of size block_size X block_size
              and data from each block is moved to the channel dimension.
              Output CHW dimensions are: [C * block_size * block_size, H/block_size, C/block_size].

            - If mode == 'DEPTH_TO_SPACE': data is moved from the channel to the spatial dimension.
              Reverse of the operation 'SPACE_TO_DEPTH'.
              Output CHW dimensions are: [C/(block_size * block_size), H * block_size, C * block_size].

        block_size: int
            Must be greater than 1. Must divide H and W, when mode is 'SPACE_TO_DEPTH'. (block_size * block_size)
            must divide C when mode is 'DEPTH_TO_SPACE'.

        See Also
        --------
        add_flatten, add_reshape
        """
        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.reorganizeData

        # Set the parameters
        if block_size < 2:
            raise ValueError("Invalid block_size value %d. Must be greater than 1." % block_size)
        spec_layer_params.blockSize = block_size

        mode = mode.upper() if isinstance(mode, str) else mode
        if mode == 'SPACE_TO_DEPTH':
            spec_layer_params.mode = \
                _NeuralNetwork_pb2.ReorganizeDataLayerParams.ReorganizationType.Value('SPACE_TO_DEPTH')
        elif mode == 'DEPTH_TO_SPACE':
            spec_layer_params.mode = \
                _NeuralNetwork_pb2.ReorganizeDataLayerParams.ReorganizationType.Value('DEPTH_TO_SPACE')
        else:
            raise NotImplementedError('Unknown reorganization mode %s.' % mode)
        return spec_layer

    def add_batchnorm(self, name, channels, gamma, beta,
                      mean=None, variance=None,
                      input_name='data', output_name='out',
                      compute_mean_var=False,
                      instance_normalization=False, epsilon=1e-5):
        """
        Add a batch normalization layer. Batch normalization operation is
        defined as:

        ``y = gamma * (x - mean) / sqrt(variance + epsilon) + beta``

        Refer to the **BatchnormLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        channels: int
            Number of channels of the input blob.
        gamma: numpy.array
            Values of gamma. Must be numpy array of shape (channels, ).
        beta: numpy.array
            Values of beta. Must be numpy array of shape (channels, ).
        mean: numpy.array
            Means of the input blob on each channel. Must be numpy array of shape (channels, ).
        variance:
            Variances of the input blob on each channel. Must be numpy array of shape (channels, ).
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        compute_mean_var: bool
            Set to True if mean and variance is to be computed from the input data.
        instance_normalization: bool
            Set compute_mean_var and this to True to perform
            instance normalization i.e., mean and variance are computed from the single input instance.
        epsilon: float
            Value of epsilon. Defaults to 1e-5 if not specified.

        See Also
        --------
        add_convolution, add_pooling, add_inner_product
        """
        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.batchnorm

        # Set the parameters
        spec_layer_params.channels = channels
        spec_layer_params.gamma.floatValue.extend(map(float, gamma.flatten()))
        spec_layer_params.beta.floatValue.extend(map(float, beta.flatten()))
        spec_layer_params.epsilon = epsilon
        spec_layer_params.computeMeanVar = compute_mean_var
        spec_layer_params.instanceNormalization = instance_normalization

        if compute_mean_var:
            if not instance_normalization:
                raise NotImplementedError('Batch-instance norm is currently not supported')

        if not compute_mean_var:
            spec_layer_params.mean.floatValue.extend(map(float, mean.flatten()))
            spec_layer_params.variance.floatValue.extend(map(float, variance.flatten()))

        return spec_layer

    def add_permute(self, name, dim, input_name, output_name):
        """
        Add a permute layer. Assumes that the input has dimensions in the order [Seq, C, H, W]
        Refer to the **PermuteLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        dim: tuple
            The order in which to permute the input dimensions = [seq,C,H,W].
            Must have length 4 and a permutation of ``[0, 1, 2, 3]``.

            examples:

            Lets say input has shape: [seq, C, H, W].

            If ``dim`` is set to ``[0, 3, 1, 2]``,
            then the output has shape ``[W,C,H]``
            and has the same sequence length that of the input.

            If ``dim`` is set to ``[3, 1, 2, 0]``,
            and the input is a sequence of data
            with length ``Seq`` and shape ``[C, 1, 1]``,
            then the output is a unit sequence of data with shape ``[C, 1, Seq]``.

            If ``dim`` is set to ``[0, 3, 2, 1]``,
            the output is a reverse of the input: ``[C, H, W] -> [W, H, C]``.

            If ``dim`` is not set, or is set to ``[0, 1, 2, 3]``,
            the output is the same as the input.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_flatten, add_reshape
        """
        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.permute
        spec_layer_params.axis.extend(list(dim))

        if len(dim) != 4:
            raise ValueError("Length of the 'dim' parameter must be equal to 4")
        return spec_layer

    def add_reshape(self, name, input_name, output_name, target_shape, mode):
        """
        Add a reshape layer.
        Refer to the **ReshapeLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        target_shape: tuple
            Shape of the output blob. The product of target_shape must be equal
            to the shape of the input blob.
            Can be either length 3 (C,H,W) or length 4 (Seq,C,H,W).
        mode: int

            - If mode == 0, the reshape layer is in CHANNEL_FIRST mode.
            - If mode == 1, the reshape layer is in CHANNEL_LAST mode.

        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_flatten, add_permute
        """
        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.reshape
        spec_layer_params.targetShape.extend(target_shape)
        if mode == 0:
            spec_layer_params.mode = \
                _NeuralNetwork_pb2.ReshapeLayerParams.ReshapeOrder.Value('CHANNEL_FIRST')
        else:
            spec_layer_params.mode = \
                _NeuralNetwork_pb2.ReshapeLayerParams.ReshapeOrder.Value('CHANNEL_LAST')

        if len(target_shape) != 4 and len(target_shape) != 3:
            raise ValueError("Length of the 'target-shape' parameter must be equal to 3 or 4")
        self.rank_dict[output_name] = len(target_shape)
        return spec_layer

    def add_reduce(self, name, input_name, output_name, axis, mode, epsilon=1e-6):
        """
        Add a reduce layer. Applies the function specified by the parameter mode,
        along dimension(s) specified by the parameter axis.
        Refer to the **ReduceLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.

        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        axis: str
            dimensions along which the reduction operation is applied.
            Allowed values: 'CHW', 'HW', 'C', 'H', 'W'

        mode: str
            Reduction operation to be applied.
            Allowed values:
            'sum', 'avg', 'prod', 'logsum', 'sumsquare', 'L1', 'L2', 'max', 'min', 'argmax'.
            'argmax' is only supported with axis values 'C', 'H' and 'W'.

        epsilon: float
            number that is added to the input when 'logsum' function is applied.

        See Also
        --------
        add_activation
        """
        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.reduce
        spec_layer_params.epsilon = epsilon

        mode = mode.lower() if isinstance(mode, str) else mode
        if mode == 'sum':
            spec_layer_params.mode = _NeuralNetwork_pb2.ReduceLayerParams.ReduceOperation.Value('SUM')
        elif mode == 'avg':
            spec_layer_params.mode = _NeuralNetwork_pb2.ReduceLayerParams.ReduceOperation.Value('AVG')
        elif mode == 'prod':
            spec_layer_params.mode = _NeuralNetwork_pb2.ReduceLayerParams.ReduceOperation.Value('PROD')
        elif mode == 'logsum':
            spec_layer_params.mode = _NeuralNetwork_pb2.ReduceLayerParams.ReduceOperation.Value('LOGSUM')
        elif mode == 'sumsquare':
            spec_layer_params.mode = _NeuralNetwork_pb2.ReduceLayerParams.ReduceOperation.Value('SUMSQUARE')
        elif mode == 'l1':
            spec_layer_params.mode = _NeuralNetwork_pb2.ReduceLayerParams.ReduceOperation.Value('L1')
        elif mode == 'l2':
            spec_layer_params.mode = _NeuralNetwork_pb2.ReduceLayerParams.ReduceOperation.Value('L2')
        elif mode == 'max':
            spec_layer_params.mode = _NeuralNetwork_pb2.ReduceLayerParams.ReduceOperation.Value('MAX')
        elif mode == 'min':
            spec_layer_params.mode = _NeuralNetwork_pb2.ReduceLayerParams.ReduceOperation.Value('MIN')
        elif mode == 'argmax':
            spec_layer_params.mode = _NeuralNetwork_pb2.ReduceLayerParams.ReduceOperation.Value('ARGMAX')
        else:
            raise NotImplementedError('Unknown reduction operation %s.' % mode)

        axis = axis.upper() if isinstance(axis, str) else axis
        if axis == 'CHW':
            spec_layer_params.axis = _NeuralNetwork_pb2.ReduceLayerParams.ReduceAxis.Value('CHW')
        elif axis == 'HW':
            spec_layer_params.axis = _NeuralNetwork_pb2.ReduceLayerParams.ReduceAxis.Value('HW')
        elif axis == 'C':
            spec_layer_params.axis = _NeuralNetwork_pb2.ReduceLayerParams.ReduceAxis.Value('C')
        elif axis == 'H':
            spec_layer_params.axis = _NeuralNetwork_pb2.ReduceLayerParams.ReduceAxis.Value('H')
        elif axis == 'W':
            spec_layer_params.axis = _NeuralNetwork_pb2.ReduceLayerParams.ReduceAxis.Value('W')
        else:
            raise NotImplementedError('Unknown reduction axis %s.' % axis)
        return spec_layer

    def add_lrn(self, name, input_name, output_name, alpha, beta, local_size, k=1.0):
        """
        Add a LRN (local response normalization) layer. Supports "across" channels normalization.
        Refer to the **LRNLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.

        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        alpha: float
            multiplicative constant in the denominator.
        beta: float
            exponent of the normalizing term in the denominator.
        k: float
            bias term in the denominator. Must be positive.
        local_size: int
            size of the neighborhood along the channel axis.

        See Also
        --------
        add_l2_normalize, add_mvn
        """
        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.lrn
        spec_layer_params.alpha = alpha
        spec_layer_params.beta = beta
        spec_layer_params.localSize = local_size
        spec_layer_params.k = k
        return spec_layer

    def add_mvn(self, name, input_name, output_name, across_channels=True, normalize_variance=True, epsilon=1e-5):
        """
        Add an MVN (mean variance normalization) layer. Computes mean, variance and normalizes the input.
        Refer to the **MeanVarianceNormalizeLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.

        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        across_channels: boolean
            If False, each channel plane is normalized separately
            If True, mean/variance is computed across all C, H and W dimensions

        normalize_variance: boolean
            If False, only mean subtraction is performed.

        epsilon: float
            small bias to avoid division by zero.


        See Also
        --------
        add_l2_normalize, add_lrn
        """
        spec_layer = self._add_generic_layer(name, [input_name], [output_name])

        spec_layer_params = spec_layer.mvn
        spec_layer_params.acrossChannels = across_channels
        spec_layer_params.normalizeVariance = normalize_variance
        spec_layer_params.epsilon = epsilon
        return spec_layer

    def add_l2_normalize(self, name, input_name, output_name, epsilon=1e-5):
        """
        Add L2 normalize layer. Normalizes the input by the L2 norm, i.e. divides by the
        the square root of the sum of squares of all elements of the input along C, H and W dimensions.
        Refer to the **L2NormalizeLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.

        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        epsilon: float
            small bias to avoid division by zero.


        See Also
        --------
        add_mvn, add_lrn
        """
        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.l2normalize
        spec_layer_params.epsilon = epsilon
        return spec_layer

    def add_unary(self, name, input_name, output_name, mode, alpha=1.0,
                  shift=0, scale=1.0, epsilon=1e-6):
        """
        Add a Unary layer. Applies the specified function (mode) to all the elements of the input.
        Prior to the application of the function the input can be scaled and shifted by using the 'scale',
        'shift' parameters.
        Refer to the **UnaryFunctionLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.

        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        mode: str
            Unary function.
            Allowed values: 'sqrt', 'rsqrt', 'inverse', 'power', 'exp', 'log', 'abs', threshold'.

        alpha: float
            constant used in with modes 'power' and 'threshold'.

        shift, scale: float
            input is modified by scale and shift prior to the application of the unary function.

        epsilon: float
            small bias to prevent division by zero.

        See Also
        --------
        add_activation
        """
        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.unary
        spec_layer_params.epsilon = epsilon
        spec_layer_params.alpha = alpha
        spec_layer_params.shift = shift
        spec_layer_params.scale = scale

        mode = mode.lower() if isinstance(mode, str) else mode
        if mode == 'sqrt':
            spec_layer_params.type = _NeuralNetwork_pb2.UnaryFunctionLayerParams.Operation.Value('SQRT')
        elif mode == 'rsqrt':
            spec_layer_params.type = _NeuralNetwork_pb2.UnaryFunctionLayerParams.Operation.Value('RSQRT')
        elif mode == 'inverse':
            spec_layer_params.type = _NeuralNetwork_pb2.UnaryFunctionLayerParams.Operation.Value('INVERSE')
        elif mode == 'power':
            spec_layer_params.type = _NeuralNetwork_pb2.UnaryFunctionLayerParams.Operation.Value('POWER')
        elif mode == 'exp':
            spec_layer_params.type = _NeuralNetwork_pb2.UnaryFunctionLayerParams.Operation.Value('EXP')
        elif mode == 'log':
            spec_layer_params.type = _NeuralNetwork_pb2.UnaryFunctionLayerParams.Operation.Value('LOG')
        elif mode == 'abs':
            spec_layer_params.type = _NeuralNetwork_pb2.UnaryFunctionLayerParams.Operation.Value('ABS')
        elif mode == 'threshold':
            spec_layer_params.type = _NeuralNetwork_pb2.UnaryFunctionLayerParams.Operation.Value('THRESHOLD')
        else:
            raise NotImplementedError('Unknown unary function %s ' % mode)
        return spec_layer

    def add_split(self, name, input_name, output_names):
        """
        Add a split layer that uniformly splits the input along the channel dimension
        to produce multiple outputs.
        Refer to the **SplitLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_names: list of str
            List of output blob names of this layer.

        See Also
        --------
        add_elementwise
        """
        spec_layer = self._add_generic_layer(name, [input_name], output_names)
        spec_layer_params = spec_layer.split
        spec_layer_params.nOutputs = len(output_names)
        return spec_layer

    def add_load_constant(self, name, output_name, constant_value, shape):
        """
        Add a load constant layer.
        Refer to the **LoadConstantLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.

        output_name: str
            The output blob name of this layer.

        constant_value: numpy.array
            value of the constant as a numpy array.

        shape: list of int or tuple of int
            List of ints representing the shape of the constant. Must be of length 3: [C,H,W]


        See Also
        --------
        add_elementwise
        """
        spec_layer = self._add_generic_layer(name, [], [output_name])
        spec_layer_params = spec_layer.loadConstant

        data = spec_layer_params.data
        data.floatValue.extend(map(float, constant_value.flatten()))

        spec_layer_params.shape.extend(shape)

        self.rank_dict[output_name] = 5
        if len(data.floatValue) != np.prod(shape):
            raise ValueError("Dimensions of 'shape' do not match the size of the provided constant")
        if not self._disable_rank5_shape_mapping:
            if len(shape) != 3:
                raise ValueError("'shape' must be of length 3")
        return spec_layer

    def add_custom(self, name, input_names, output_names, custom_proto_spec=None):
        """
        Add a custom layer.
        Refer to the **CustomLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.

        input_names: list of str
            The input blob names to this layer.

        output_names: list of str
            The output blob names from this layer.

        custom_proto_spec: CustomLayerParams
            A protobuf CustomLayerParams message. This can also be left blank and filled in later.
        """
        # custom layers require a newer specification version
        from coremltools import _MINIMUM_CUSTOM_LAYER_SPEC_VERSION
        if self.spec:
            self.spec.specificationVersion = max(self.spec.specificationVersion, _MINIMUM_CUSTOM_LAYER_SPEC_VERSION)

        spec_layer = self._add_generic_layer(name, input_names, output_names)

        spec_layer.custom.MergeFromString(b'')
        if custom_proto_spec:
            spec_layer.custom.CopyFrom(custom_proto_spec)
        return spec_layer

    def add_resize_bilinear(self, name, input_name, output_name, target_height=1, target_width=1,
                            mode='ALIGN_ENDPOINTS_MODE'):
        """
        Add a resize bilinear layer to the model. A layer that resize the input to a given spatial size using bilinear interpolation.
        Refer to the **ResizeBilinearLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        target_height: int
            Output height dimension.
        target_width: int
            Output width dimension.
        mode: str
            Following values are supported: 'STRICT_ALIGN_ENDPOINTS_MODE', 'ALIGN_ENDPOINTS_MODE', 'UPSAMPLE_MODE', 'ROI_ALIGN_MODE'.
            This parameter determines the sampling grid used for bilinear interpolation.

        See Also
        --------
        add_upsample
        """
        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.resizeBilinear
        spec_layer_params.targetSize.append(target_height)
        spec_layer_params.targetSize.append(target_width)
        mode = mode.upper() if isinstance(mode, str) else mode
        if mode == 'ALIGN_ENDPOINTS_MODE':
            spec_layer_params.mode.samplingMethod = _NeuralNetwork_pb2.SamplingMode.Method.Value('ALIGN_ENDPOINTS_MODE')
        elif mode == 'STRICT_ALIGN_ENDPOINTS_MODE':
            spec_layer_params.mode.samplingMethod = _NeuralNetwork_pb2.SamplingMode.Method.Value(
                'STRICT_ALIGN_ENDPOINTS_MODE')
        elif mode == 'UPSAMPLE_MODE':
            spec_layer_params.mode.samplingMethod = _NeuralNetwork_pb2.SamplingMode.Method.Value('UPSAMPLE_MODE')
        elif mode == 'ROI_ALIGN_MODE':
            spec_layer_params.mode.samplingMethod = _NeuralNetwork_pb2.SamplingMode.Method.Value('ROI_ALIGN_MODE')
        else:
            raise ValueError("Unsupported resize bilinear mode %s" % mode)
        return spec_layer

    def add_crop_resize(self, name, input_names, output_name, target_height=1, target_width=1,
                        mode='STRICT_ALIGN_ENDPOINTS_MODE',
                        normalized_roi=False,
                        box_indices_mode='CORNERS_HEIGHT_FIRST',
                        spatial_scale=1.0):
        """
        Add a crop resize layer to the model. A layer that extracts cropped spatial patches or RoIs (regions of interest)
        from the input and resizes them to a pre-specified size using bilinear interpolation.
        Note that RoI Align layer can be implemented with this layer followed by a pooling layer.
        Refer to the **CropResizeLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            Must be a list of two names: image feature map and crop indices/RoI input.
            First input corresponds to a blob with shape ``[1, Batch, C, H_in, W_in]``. This represents a batch of input image feature data with C channels.
            The second input shape must be ``[N, 1, 4, 1, 1]`` or ``[N, 1, 5, 1, 1]``. This represents the bounding box coordinates for N patches/RoIs.
            N: number of patches/RoIs to be extracted
            If RoI shape = [N, 1, 4, 1, 1]
            The channel axis corresponds to the four coordinates specifying the bounding box.
            All the N~ RoIs are extracted from all the batches of the input.
            If RoI shape = [N, 1, 5, 1, 1]
            The first element of the channel axis specifies the input batch id from which to extract the RoI and
            must be in the interval ``[0, Batch - 1]``. That is, n-th RoI is extracted from the RoI[n,0,0,0]-th input batch id.
            The last four elements of the channel axis specify the bounding box coordinates.
        output_name: str
            The output blob name of this layer.
        target_height: int
            Output height dimension.
        target_width: int
            Output width dimension.
        mode: str
            Following values are supported: 'STRICT_ALIGN_ENDPOINTS_MODE', 'ALIGN_ENDPOINTS_MODE', 'UPSAMPLE_MODE', 'ROI_ALIGN_MODE'.
            This parameter determines the sampling grid used for bilinear interpolation.
        normalized_roi: bool
            If true the bounding box coordinates must be in the interval [0, 1].
            They are scaled by (input_height - 1), (input_width - 1), i.e. based on the input spatial dimensions.
            If false the bounding box coordinates must be in the interval
            [0, input_height - 1] and [0, input_width - 1], respectively for height and width dimensions.
        box_indices_mode: str
            Following values are supported: 'CORNERS_HEIGHT_FIRST', 'CORNERS_WIDTH_FIRST', 'CENTER_SIZE_HEIGHT_FIRST', 'CENTER_SIZE_WIDTH_FIRST'
            Representation used to interpret the bounding box coordinates (RoI) input.
            'CORNERS_HEIGHT_FIRST': [h_start, w_start, h_end, w_end]
            'CORNERS_WIDTH_FIRST': [w_start, h_start, w_end, h_end]
            'CENTER_SIZE_HEIGHT_FIRST': [h_center, w_center, box_height, box_width]
            'CENTER_SIZE_WIDTH_FIRST': [w_center, h_center, box_width, box_height]
        spatial_scale: float
            Additional spatial scale that multiplies the bounding box coordinates.
            Generally used while implementing the RoI Align layer,
            which uses unnormalized RoI coordinates along with a spatial scale less than or equal to 1.

        See Also
        --------
        add_resize_bilinear, add_crop
        """
        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer_params = spec_layer.cropResize
        spec_layer_params.targetSize.append(target_height)
        spec_layer_params.targetSize.append(target_width)
        spec_layer_params.normalizedCoordinates = normalized_roi
        spec_layer_params.spatialScale = spatial_scale

        mode = mode.upper() if isinstance(mode, str) else mode
        box_indices_mode = box_indices_mode.upper() if isinstance(box_indices_mode, str) else box_indices_mode

        if mode == 'ALIGN_ENDPOINTS_MODE':
            spec_layer_params.mode.samplingMethod = _NeuralNetwork_pb2.SamplingMode.Method.Value('ALIGN_ENDPOINTS_MODE')
        elif mode == 'STRICT_ALIGN_ENDPOINTS_MODE':
            spec_layer_params.mode.samplingMethod = _NeuralNetwork_pb2.SamplingMode.Method.Value(
                'STRICT_ALIGN_ENDPOINTS_MODE')
        elif mode == 'UPSAMPLE_MODE':
            spec_layer_params.mode.samplingMethod = _NeuralNetwork_pb2.SamplingMode.Method.Value('UPSAMPLE_MODE')
        elif mode == 'ROI_ALIGN_MODE':
            spec_layer_params.mode.samplingMethod = _NeuralNetwork_pb2.SamplingMode.Method.Value('ROI_ALIGN_MODE')
        else:
            raise ValueError("Unsupported crop resize mode %s" % mode)

        if box_indices_mode == 'CORNERS_HEIGHT_FIRST':
            spec_layer_params.boxIndicesMode.boxMode = _NeuralNetwork_pb2.BoxCoordinatesMode.Coordinates.Value(
                'CORNERS_HEIGHT_FIRST')
        elif box_indices_mode == 'CORNERS_WIDTH_FIRST':
            spec_layer_params.boxIndicesMode.boxMode = _NeuralNetwork_pb2.BoxCoordinatesMode.Coordinates.Value(
                'CORNERS_WIDTH_FIRST')
        elif box_indices_mode == 'CENTER_SIZE_HEIGHT_FIRST':
            spec_layer_params.boxIndicesMode.boxMode = _NeuralNetwork_pb2.BoxCoordinatesMode.Coordinates.Value(
                'CENTER_SIZE_HEIGHT_FIRST')
        elif box_indices_mode == 'CENTER_SIZE_WIDTH_FIRST':
            spec_layer_params.boxIndicesMode.boxMode = _NeuralNetwork_pb2.BoxCoordinatesMode.Coordinates.Value(
                'CENTER_SIZE_WIDTH_FIRST')
        else:
            raise ValueError("Unsupported crop resize box indices mode %s" % box_indices_mode)
        return spec_layer

    def set_pre_processing_parameters(self, image_input_names=None, is_bgr=False,
                                      red_bias=0.0, green_bias=0.0, blue_bias=0.0, gray_bias=0.0, image_scale=1.0,
                                      image_format='NCHW'):
        """
        Add a pre-processing parameters layer to the neural network object.

        Parameters
        ----------
        image_input_names: list of str
            Name of input blobs that are images

        is_bgr: boolean or dict()
            Channel order for input blobs that are images. BGR if True else RGB.
            To specify a different value for each image input,
            provide a dictionary with input names as keys.

        red_bias: float or dict()
            Image re-centering parameter (red channel)

        blue_bias: float or dict()
            Image re-centering parameter (blue channel)

        green_bias: float or dict()
            Image re-centering parameter (green channel)

        gray_bias: float or dict()
            Image re-centering parameter (for grayscale images)

        image_scale: float or dict()
            Value by which to scale the images.
        
        image_format: str
            Image format, either 'NCHW' / 'NHWC'

        See Also
        --------
        set_input, set_output, set_class_labels
        """
        spec = self.spec
        if not image_input_names:
            return  # nothing to do here

        image_format = image_format.upper() if isinstance(image_format, str) else image_format
        if image_format != 'NCHW' and image_format != 'NHWC':
            raise ValueError("Input image format must be either 'NCHW' or 'NHWC'. Provided {}".format(image_format))

        if not isinstance(is_bgr, dict):
            is_bgr = dict.fromkeys(image_input_names, is_bgr)
        if not isinstance(red_bias, dict):
            red_bias = dict.fromkeys(image_input_names, red_bias)
        if not isinstance(blue_bias, dict):
            blue_bias = dict.fromkeys(image_input_names, blue_bias)
        if not isinstance(green_bias, dict):
            green_bias = dict.fromkeys(image_input_names, green_bias)
        if not isinstance(gray_bias, dict):
            gray_bias = dict.fromkeys(image_input_names, gray_bias)
        if not isinstance(image_scale, dict):
            image_scale = dict.fromkeys(image_input_names, image_scale)

        # Add image inputs
        for input_ in spec.description.input:
            if input_.name in image_input_names:
                if input_.type.WhichOneof('Type') == 'multiArrayType':
                    array_shape = tuple(input_.type.multiArrayType.shape)

                    if len(array_shape) == 4:
                        input_indices = [0, 1, 2, 3] if image_format == 'NCHW' else [0, 3, 1, 2]
                    elif len(array_shape) == 3:
                        # Adding dummy index for 'batch' for compatibility
                        input_indices = [0, 0, 1, 2] if image_format == 'NCHW' else [0, 2, 0, 1]
                    else:
                        raise ValueError("Invalid input shape. Input of rank {}, but expecting input of either rank 3 or rank 4".format(len(array_shape)))

                    # Extract image shape depending on input format
                    _, channels, height, width = [array_shape[e] for e in input_indices]

                    if image_format == 'NHWC':
                        # If input format is 'NHWC', then add transpose
                        # after the input and replace all use of input
                        # with output of transpose
                        axes = [1, 2, 0]
                        if len(array_shape) == 4:
                            axes = [0, 2, 3, 1]
                        input_transpose = input_.name + '_to_nhwc'
                        transpose_layer = self.add_transpose(
                            name=input_transpose,
                            axes=axes,
                            input_name=input_.name,
                            output_name=input_transpose
                        )
                        layers = spec.neuralNetwork.layers
                        layers.insert(0, layers.pop())
                        for layer_ in layers:
                            for i in range(len(layer_.input)):
                                if layer_.name == input_transpose:
                                    continue
                                if layer_.input[i] == input_.name:
                                    layer_.input[i] = input_transpose

                    # TODO: If input is not rank 3 or 4, then accordingly handle
                    # e.g. for rank-2 input, squeeze additional dimension in case of Gray scale image
                    if channels == 1:
                        input_.type.imageType.colorSpace = _FeatureTypes_pb2.ImageFeatureType.ColorSpace.Value(
                            'GRAYSCALE')
                    elif channels == 3:
                        if input_.name in is_bgr:
                            if is_bgr[input_.name]:
                                input_.type.imageType.colorSpace = _FeatureTypes_pb2.ImageFeatureType.ColorSpace.Value(
                                    'BGR')
                            else:
                                input_.type.imageType.colorSpace = _FeatureTypes_pb2.ImageFeatureType.ColorSpace.Value(
                                    'RGB')
                        else:
                            input_.type.imageType.colorSpace = _FeatureTypes_pb2.ImageFeatureType.ColorSpace.Value(
                                'RGB')
                    else:
                        raise ValueError("Channel Value %d not supported for image inputs" % channels)
                    input_.type.imageType.width = width
                    input_.type.imageType.height = height

                preprocessing = self.nn_spec.preprocessing.add()
                preprocessing.featureName = input_.name
                scaler = preprocessing.scaler
                if input_.name in image_scale:
                    scaler.channelScale = image_scale[input_.name]
                else:
                    scaler.channelScale = 1.0
                if input_.name in red_bias: scaler.redBias = red_bias[input_.name]
                if input_.name in blue_bias: scaler.blueBias = blue_bias[input_.name]
                if input_.name in green_bias: scaler.greenBias = green_bias[input_.name]
                if input_.name in gray_bias: scaler.grayBias = gray_bias[input_.name]

    def add_transpose(self, name, axes, input_name, output_name):
        """
        Add a N-D transpose layer with axes as a parameter.
        Refer to the **TransposeLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.

        axes: list of int or tuple of int
            The list containing a permutation of "[0,1,2,...,N-1]" where N is the rank of input/output tensor.

        input_name: str
            The input blob name of this layer.

        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_permute, add_reshape
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])

        rank = len(axes)
        axes = [rank + axis if axis < 0 else axis for axis in axes]
        spec_layer.transpose.axes.extend(axes)

        return spec_layer

    def add_softmax_nd(self, name, input_name, output_name, axis):
        """
        Add a softmax_nd layer to the model that performs softmax operation along
        the given axis.
        Refer to the **SoftmaxNDLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        axis: int
            Axis to perform the softmax operation on.
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.softmaxND
        spec_layer_params.axis = axis
        return spec_layer

    def add_concat_nd(self, name, input_names, output_name, axis):
        """
        Add a concat_nd layer to the model that performs concatenation along the
        given axis.
        Refer to the **ConcatNDLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        axis: int
            Axis to perform the concat operation on.
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer_params = spec_layer.concatND
        spec_layer_params.axis = axis
        return spec_layer

    def add_erf(self, name, input_name, output_name):
        """
        Add an erf function (gaussian error function) layer to the model.
        Refer to the **ErfLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.erf.MergeFromString(b'')
        return spec_layer

    def add_gelu(self, name, input_name, output_name, mode='EXACT'):
        """
        Add a GELU (gaussian error linear unit) activation layer, which is:
        ``0.5 * x * (1 + erf(x / sqrt(2)))``.
        Refer to the **GeluLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        mode: str, optional
            Gelu mode in [EXACT | TANH_APPROXIMATION | SIGMOID_APPROXIMATION], default EXACT.
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.gelu

        if mode == 'EXACT':
            spec_layer_params.mode = _NeuralNetwork_pb2.GeluLayerParams.GeluMode.Value('EXACT')
        elif mode == 'TANH_APPROXIMATION':
            spec_layer_params.mode = _NeuralNetwork_pb2.GeluLayerParams.GeluMode.Value('TANH_APPROXIMATION')
        elif mode == 'SIGMOID_APPROXIMATION':
            spec_layer_params.mode = _NeuralNetwork_pb2.GeluLayerParams.GeluMode.Value('SIGMOID_APPROXIMATION')
        else:
            raise ValueError("Unsupported Gelu mode %s" % mode)
        return spec_layer

    def add_sin(self, name, input_name, output_name):
        """
        Add a sin layer to the model that computes element-wise sine for the
        input tensor.
        Refer to the **SinLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_sinh, add_asin, add_asinh
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.sin.MergeFromString(b'')
        return spec_layer

    def add_cos(self, name, input_name, output_name):
        """
        Add a cos layer to the model that computes element-wise cosine for the
        input tensor.
        Refer to the **CosLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_cosh, add_acos, add_acosh
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.cos.MergeFromString(b'')
        return spec_layer

    def add_tan(self, name, input_name, output_name):
        """
        Add a tan layer to the model that computes element-wise tangent for the
        input tensor.
        Refer to the **TanLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_tanh, add_atan, add_atanh
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.tan.MergeFromString(b'')
        return spec_layer

    def add_asin(self, name, input_name, output_name):
        """
        Add an asin layer to the model that computes element-wise arc-sine for
        the input tensor.
        Refer to the **AsinLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_sin, add_sinh, add_asinh
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.asin.MergeFromString(b'')
        return spec_layer

    def add_acos(self, name, input_name, output_name):
        """
        Add an acos layer to the model that computes element-wise arc-cosine
        for the input tensor.
        Refer to the **AcosLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_cos, add_cosh, add_acosh
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.acos.MergeFromString(b'')
        return spec_layer

    def add_atan(self, name, input_name, output_name):
        """
        Add an atan layer to the model that computes element-wise arc-tangent
        for the input tensor.
        Refer to the **AtanLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_tan, add_tanh, add_atanh
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.atan.MergeFromString(b'')
        return spec_layer

    def add_sinh(self, name, input_name, output_name):
        """
        Add a sinh layer to the model that computes element-wise hyperbolic sine for the input tensor.
        Refer to the **SinhLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_sin, add_asin, add_asinh
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.sinh.MergeFromString(b'')
        return spec_layer

    def add_cosh(self, name, input_name, output_name):
        """
        Add a osh layer to the model that computes element-wise hyperbolic
        cosine for the input tensor.
        Refer to the **CoshLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_cos, add_acos, add_acosh
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.cosh.MergeFromString(b'')
        return spec_layer

    def add_tanh(self, name, input_name, output_name):
        """
        Add a tanh layer to the model that computes element-wise hyperbolic
        tangent for the input tensor.
        Refer to the **TanhLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_tan, add_atan, add_atanh
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.tanh.MergeFromString(b'')
        return spec_layer

    def add_asinh(self, name, input_name, output_name):
        """
        Add an asinh layer to the model that computes element-wise inverse
        hyperbolic sine for the input tensor.
        Refer to the **AsinhLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_sin, add_sinh, add_asin
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.asinh.MergeFromString(b'')
        return spec_layer

    def add_acosh(self, name, input_name, output_name):
        """
        Add an acosh layer to the model that computes element-wise inverse
        hyperbolic cosine for the input tensor.
        Refer to the **AcoshLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_cos, add_cosh, add_acos
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.acosh.MergeFromString(b'')
        return spec_layer

    def add_atanh(self, name, input_name, output_name):
        """
        Add an atanh layer to the model that computes element-wise inverse
        hyperbolic tangent for the input tensor.
        Refer to the **AtanhLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_tan, add_tanh, add_atan
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.atanh.MergeFromString(b'')
        return spec_layer

    def add_exp2(self, name, input_name, output_name):
        """
        Add an exp2 layer to the model that performs element-wise experiential operation.
        Refer to the **Exp2LayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.exp2.MergeFromString(b'')
        return spec_layer

    def add_add_broadcastable(self, name, input_names, output_name):
        """
        Add an add_broadcastable layer to the model that performs element-wise
        addition operation with broadcast support.
        Refer to the **AddBroadcastableLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.addBroadcastable.MergeFromString(b'')
        self._set_max_input_rank(input_names, output_name)
        return spec_layer

    def add_multiply_broadcastable(self, name, input_names, output_name):
        """
        Add a multiply_broadcastable layer to the model that performs element-wise
        multiplication operation with broadcast support.
        Refer to the **MultiplyBroadcastableLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.multiplyBroadcastable.MergeFromString(b'')
        self._set_max_input_rank(input_names, output_name)
        return spec_layer

    def add_divide_broadcastable(self, name, input_names, output_name):
        """
        Add a divide_broadcastable layer to the model that performs element-wise
        division operation with broadcast support.
        Refer to the **DivideBroadcastableLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.divideBroadcastable.MergeFromString(b'')
        self._set_max_input_rank(input_names, output_name)
        return spec_layer

    def add_subtract_broadcastable(self, name, input_names, output_name):
        """
        Add a subtract_broadcastable layer to the model that performs element-wise
        subtraction operation with broadcast support.
        Refer to the **SubtractBroadcastableLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.subtractBroadcastable.MergeFromString(b'')
        self._set_max_input_rank(input_names, output_name)
        return spec_layer

    def add_max_broadcastable(self, name, input_names, output_name):
        """
        Add a max_broadcastable layer to the model that performs element-wise
        maximum operation with broadcast support.
        Refer to the **MaxBroadcastableLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.maxBroadcastable.MergeFromString(b'')
        self._set_max_input_rank(input_names, output_name)
        return spec_layer

    def add_min_broadcastable(self, name, input_names, output_name):
        """
        Add a min_broadcastable layer to the model that performs element-wise
        minimum operation with broadcast support.
        Refer to the **MinBroadcastableLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.minBroadcastable.MergeFromString(b'')
        self._set_max_input_rank(input_names, output_name)
        return spec_layer

    def add_floor_div_broadcastable(self, name, input_names, output_name):
        """
        Add a floor_div_broadcastable layer to the model that performs floor
        division operation with broadcast support.
        Refer to the **FloorDivBroadcastableLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_divide_broadcastable
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.floorDivBroadcastable.MergeFromString(b'')
        self._set_max_input_rank(input_names, output_name)
        return spec_layer

    def add_mod_broadcastable(self, name, input_names, output_name):
        """
        Add a mod_broadcastable layer to the model that performs element-wise
        modular operation with broadcast support.
        Refer to the **ModBroadcastableLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.modBroadcastable.MergeFromString(b'')
        self._set_max_input_rank(input_names, output_name)
        return spec_layer

    def add_pow_broadcastable(self, name, input_names, output_name):
        """
        Add a pow_broadcastable layer to the model that performs element-wise
        power operation with broadcast support.
        Refer to the **PowBroadcastableLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.powBroadcastable.MergeFromString(b'')
        self._set_max_input_rank(input_names, output_name)
        return spec_layer

    def add_stack(self, name, input_names, output_name, axis=0):
        """
        Add a stack layer to the model that performs stack operation on a list of
        tensors into one rank+1 tensor on the given axis.
        Refer to the **StackLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        axis: int, optional
            The axis to perform stack operation, default: 0.
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.stack.axis = axis
        self.rank_dict[output_name] = self._get_rank(input_names[0]) + 1
        return spec_layer

    def add_ceil(self, name, input_name, output_name):
        """
        Add a ceil layer to the model that performs element-wise ceil operation
        on the input tensor that rounds the value to the smallest integer not
        less than x.
        Refer to the **CeilLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_floor, add_clip
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.ceil.MergeFromString(b'')
        return spec_layer

    def add_floor(self, name, input_name, output_name):
        """
        Add a floor layer to the model that performs element-wise floor operation
        on the input tensor that rounds the value to the largest integer not
        greater than x.
        Refer to the **FloorLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_ceil, add_clip
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.floor.MergeFromString(b'')
        return spec_layer

    def add_round(self, name, input_name, output_name):
        """
        Add a round layer to the model that performs element-wise round operation
        on the input tensor that rounds the value to the nearest integer.
        Refer to the **RoundLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.round.MergeFromString(b'')
        return spec_layer

    def add_sign(self, name, input_name, output_name):
        """
        Add a sign layer to the model that performs element-wise sign operation
        (+1 for positive values, -1 for negative values, 0 for zeroes).
        Refer to the **SignLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.sign.MergeFromString(b'')
        return spec_layer

    def add_clip(self, name, input_name, output_name, min_value=0., max_value=1.):
        """
        Add a clip layer to the model that performs element-wise clip operation.
        Clip the values in the input tensor to the range [min_value, max_value].
        Refer to the **ClipLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        min_value: float, optional
            Lower bound / minimum value for clip, default: 0.0.
        max_value: float, optional
            Upper bound / maximum value for clip, default: 1.0.

        See Also
        --------
        add_floor, add_ceil
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.clip.MergeFromString(b'')
        spec_params = spec_layer.clip

        spec_params.minVal = float(min_value)
        spec_params.maxVal = float(max_value)

        return spec_layer

    def add_split_nd(self, name, input_name, output_names, axis, num_splits=2, split_sizes=None):
        """
        Add a split layer to the model that splits the input tensor into multiple
        output tensors. Either uniformly split the input tensor into ``num_splits``
        tensors, or split into given size list ``split_sizes`` output tensors.
        Refer to the **SplitNDLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_names: list of str
            The output blob names of this layer.
        axis: int
            Axis to perform split on.
        num_splits: int, optional
            Number of splits, default: 2.
        split_sizes: list of int or tuple of int, optional
            List of size to split, default [] or None.
        """

        if not split_sizes:
            split_sizes = []

        spec_layer = self._add_generic_layer(name, [input_name], output_names)
        spec_layer_params = spec_layer.splitND
        spec_layer_params.axis = axis

        if split_sizes and len(split_sizes) > 0:
            spec_layer_params.splitSizes.extend(split_sizes)
            spec_layer_params.numSplits = len(split_sizes)
        else:
            spec_layer_params.numSplits = num_splits

        assert len(output_names) == spec_layer_params.numSplits
        return spec_layer

    def add_slice_static(self, name, input_name, output_name, begin_ids,
                         end_ids, strides, begin_masks, end_masks):
        """
        Add a slice_static layer to the model that extracts a slice of size
        ``(end - begin) / stride`` from the given input tensor.
        Refer to the **SliceStaticLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        begin_ids: list of int or tuple of int
            Begin offsets for slice layer.
        end_ids: list of int or tuple of int
            End offsets for slice layer.
        strides: list of int or tuple of int
            Strides for slice layer.
        begin_masks: list of bool
            Boolean masks for begin offsets.
        end_masks: list of bool
            Boolean masks for end offsets.

        See Also
        --------
        add_slice_dynamic
        """

        rank = len(begin_ids)
        assert len(end_ids) == rank
        assert len(strides) == rank
        assert len(begin_masks) == rank
        assert len(end_masks) == rank

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.sliceStatic

        spec_layer_params.beginIds.extend(begin_ids)
        spec_layer_params.endIds.extend(end_ids)
        spec_layer_params.strides.extend(strides)
        spec_layer_params.beginMasks.extend(begin_masks)
        spec_layer_params.endMasks.extend(end_masks)

        return spec_layer

    def add_slice_dynamic(self, name, input_names, output_name, end_ids=None,
                          strides=None, begin_masks=None, end_masks=None):
        """
        Add a slice_dynamic layer to the model that extracts a slice of size
        ``(end - begin) / stride`` from the given input tensor.
        Refer to the **SliceDynamicLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        end_ids: list of int or tuple of int, optional
            End offsets for slice layer, default: [1].
        strides: list of int or tuple of int, optional
            Strides for slice layer, default: [1].
        begin_masks: list of bool, optional
            Boolean masks for begin offsets, default: [false].
        end_masks: list of bool, optional
            Boolean masks for end offsets, default: [false].

        See Also
        --------
        add_slice_static
        """

        if not end_ids:
            end_ids = [1 for _ in range(5)]
        if not strides:
            strides = [1 for _ in range(5)]
        if not begin_masks:
            begin_masks = [False for _ in range(5)]
        if not end_masks:
            end_masks = [False for _ in range(5)]

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer_params = spec_layer.sliceDynamic

        spec_layer_params.endIds.extend(end_ids)
        spec_layer_params.strides.extend(strides)
        spec_layer_params.beginMasks.extend(begin_masks)
        spec_layer_params.endMasks.extend(end_masks)

        return spec_layer

    def add_tile(self, name, input_name, output_name, reps):
        """
        Add a tile layer to the model that construct a tensor by repeating the
        input tensor multiple number of times.
        Refer to the **TileLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        reps: list of int or tuple of int
            Number of times to replicate.

        See Also
        --------
        add_stack, add_concat_nd
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])

        spec_layer_params = spec_layer.tile
        assert all([i > 0 for i in reps])
        spec_layer_params.reps.extend(reps)
        return spec_layer

    def add_range_static(self, name, output_name, input_names=None, end=1, start=0, step=1):
        """
        Add a range_static layer that returns a tensor that contains evenly spaced values.
        This layer has no input and three parameters.
        Refer to the **RangeStaticLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        output_name: str
            The output blob name of this layer.
        input_names: list of str
            The input blob names of this layer.
        end: int, optional
            Range parameter: end, default: 1.
        start: int, optional
            Range parameter: start, default: 0.
        step: int, optional
            Range parameter: step size, default: 1.

        See Also
        --------
        add_range_dynamic
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.rangeStatic.MergeFromString(b'')
        spec_params = spec_layer.rangeStatic

        spec_params.endValue = float(end)
        spec_params.startValue = float(start)
        spec_params.stepSizeValue = float(step)

        self.rank_dict[output_name] = 1
        return spec_layer

    def add_range_dynamic(self, name, input_names, output_name, start=0, step=1):
        """
        Add a range_dynamic layer that returns a tensor that contains evenly spaced values.
        This layer has up to three inputs or no input and three parameters.
        Refer to the **RangeDynamicLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names.
            If input size == 1: end is input, start and step are read from parameters
            If input size == 2: end, start are inputs, step is read from parameters
            If input size == 3: start, end, step are all inputs, none of the parameters are used.
        output_name: str
            The output blob name of this layer.
        start: int, optional
            Range parameter: start. Ignored if start is provided as input, default: 0.
        step: int, optional
            Range parameter: step. Ignored if step is provided as input, default: 1.

        See Also
        --------
        add_range_static
        """

        if len(input_names) < 1 or len(input_names) > 3:
            raise ValueError('RangeDynamic layer must have either 1, 2 or 3 inputs.')

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.rangeDynamic.MergeFromString(b'')
        spec_params = spec_layer.rangeDynamic

        spec_params.startValue = float(start)
        spec_params.stepSizeValue = float(step)

        self.rank_dict[output_name] = 1
        return spec_layer

    def add_branch(self, name, input_name, if_branch=None, else_branch=None):
        """
        Add a branch layer to the model that provides the functionality of
        branching or an ``if-else`` block.
        Refer to the **BranchLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        if_branch: NeuralNetwork
            Neural network to execute if the absolute value of the input tensor is greater than 1e-6.
        else_branch: NeuralNetwork, optional
            Neural network to execute if the absolute value of the input tensor is less than 1e-6.

        See Also
        --------
        add_loop, add_loop_continue, add_loop_break
        """

        layer = self._add_generic_layer(name, [input_name], [])
        branch = layer.branch
        if if_branch:
            branch.ifBranch = if_branch
        else:
            branch.ifBranch.MergeFromString(b'')
        if else_branch:
            branch.elseBranch = else_branch
        else:
            branch.elseBranch.MergeFromString(b'')
        return layer

    def add_loop(self, name, body_network=None, input_name=None, condition=None,
                 condition_network=None, max_iterations=None):
        """
        Add a loop layer to the model that provides the functionality of a ``for``
        loop, or a ``while`` loop.
        Refer to the **LoopLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        body_network: NeuralNetwork
            Neural network to execute for the body of the loop.
        input_name: str
            The input blob name of this layer.
        condition: str, optional
            Condition of the loop.
        condition_network: NeuralNetwork, optional
            Neural network to execute for the condition of the loop.
        max_iterations: int, optional
            Maximum number of iterations of the loop.

        See Also
        --------
        add_loop_break, add_loop_continue, add_branch
        """

        input_names = [] if input_name is None else [input_name]
        spec_layer = self._add_generic_layer(name, input_names, [])
        loop = spec_layer.loop
        if condition_network is None:
            loop.conditionNetwork.MergeFromString(b'')
        else:
            loop.conditionNetwork = condition_network

        if condition is not None:
            loop.conditionVar = str(condition)
        if max_iterations is not None:
            loop.maxLoopIterations = max_iterations if max_iterations is not None else -1

        if body_network is None:
            loop.bodyNetwork.MergeFromString(b'')
        else:
            loop.bodyNetwork = body_network
        return spec_layer

    def add_loop_break(self, name):
        """
        Add a loop_break layer to the model that terminates the loop that
        contains this layer. Must reside in the ``bodyNetwork`` of the loop layer.
        Refer to the **LoopBreakLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.

        See Also
        --------
        add_loop, add_loop_continue, add_branch
        """

        spec_layer = self.nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.loopBreak.MergeFromString(b'')
        return spec_layer

    def add_loop_continue(self, name):
        """
        Add a loop_continue layer to the model that stops the current loop
        iteration and continue on the next iteration. Must reside in the
        ``bodyNetwork`` of the loop layer.
        Refer to the **LoopContinueLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.

        See Also
        --------
        add_loop, add_loop_break, add_branch
        """

        spec_layer = self.nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.loopContinue.MergeFromString(b'')
        return spec_layer

    def add_copy(self, name, input_name, output_name):
        """
        Add a copy layer to the model that copies its input tensor to the output
        tensor. Input tensor and output tensor must have distinct names.
        Refer to the **CopyLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.copy.MergeFromString(b'')
        # If output name rank is different than earlier,
        # mark it as unknown
        if output_name in self.rank_dict and self._get_rank(output_name) != self._get_rank(input_name):
            self.rank_dict[output_name] = -1
        else:
            self.rank_dict[output_name] = self._get_rank(input_name)
        return spec_layer

    def add_greater_than(self, name, input_names, output_name, use_greater_than_equal=False, alpha=0.):
        """
        Add a greater_than layer to the model that performs the element-wise
        greater-than (>) operation or greater-than-or-equal-to (>=) operation.
        Broadcasting is supported.
        Refer to the **GreaterThanLayerParams**, **GreaterEqualLayerParams** messages
        in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        use_greater_than_equal: bool, optional
            Whether or not to allow greater than or equal to, default: false.
        alpha: float, optional
            y = x1 != alpha, if only one input is provided, default: 0.

        See Also
        --------
        add_equal, add_not_equal, add_less_than
        """

        if isinstance(input_names, str):
            input_names = [input_names]
        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        if use_greater_than_equal:
            spec_layer.greaterEqual.MergeFromString(b'')
            if len(input_names) == 1:
                spec_layer.greaterEqual.alpha = alpha
        else:
            spec_layer.greaterThan.MergeFromString(b'')
            if len(input_names) == 1:
                spec_layer.greaterThan.alpha = alpha

        return spec_layer

    def add_less_than(self, name, input_names, output_name, use_less_than_equal=False, alpha=0.):
        """
        Add a less_than layer to the model that performs the element-wise
        less-than (<) operation or less-than-or-equal-to (<=) operation.
        Broadcasting is supported.
        Refer to the **LessThanL_ayerParams**, **LessEqualLayerParams** messages in
        specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        use_less_than_equal: bool, optional
            Whether or not to allow less than or equal to, default: false.
        alpha: float, optional
            y = x1 != alpha, if only one input is provided, default: 0.

        See Also
        --------
        add_equal, add_not_equal, add_greater_than
        """

        if isinstance(input_names, str):
            input_names = [input_names]
        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        if use_less_than_equal:
            spec_layer.lessEqual.MergeFromString(b'')
            if len(input_names) == 1:
                spec_layer.lessEqual.alpha = alpha
        else:
            spec_layer.lessThan.MergeFromString(b'')
            if len(input_names) == 1:
                spec_layer.lessThan.alpha = alpha
        return spec_layer

    def add_equal(self, name, input_names, output_name, alpha=0.):
        """
        Add an equal layer to the model that performs the element-wise equal
        (=) operation. Broadcasting is supported.
        Refer to the **EqualLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        alpha: float, optional
            y = x1 != alpha, if only one input is provided, default: 0.

        See Also
        --------
        add_not_equal, add_greater_than, add_less_than
        """

        if isinstance(input_names, str):
            input_names = [input_names]
        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.equal.MergeFromString(b'')
        if len(input_names) == 1:
            spec_layer.equal.alpha = alpha
        return spec_layer

    def add_not_equal(self, name, input_names, output_name, alpha=0.):
        """
        Add a not_equal layer to the model that performs the element-wise not
        equal (!=) operation. Broadcasting is supported.
        Refer to the **NotEqualLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        alpha: float, optional
            y = x1 != alpha, if only one input is provided, default: 0.

        See Also
        --------
        add_equal, add_greater_than, add_less_than
        """

        if isinstance(input_names, str):
            input_names = [input_names]
        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.notEqual.MergeFromString(b'')
        if len(input_names) == 1:
            spec_layer.notEqual.alpha = alpha
        return spec_layer

    def add_logical(self, name, input_names, output_name, mode):
        """
        Add a logical layer to the model that performs element-wise logical
        and/or/xor/not operation. Broadcasting is supported.
        Refer to the **LogicalOrLayerParams, LogicalNotLayerParams,
        LogicalNotLayerParams, LogicalAndLayerParam** messages in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        mode: str
            Logical operation mode in [AND | OR | XOR | NOT].
        """

        if isinstance(input_names, str):
            input_names = [input_names]

        spec_layer = self._add_generic_layer(name, input_names, [output_name])

        if mode in ['AND', 'OR', 'XOR'] and len(input_names) != 2:
            raise ValueError('Logical operation "%s" requires 2 inputs' % name)
        if mode in ['NOT'] and len(input_names) != 1:
            raise ValueError('Logical operation "%s" requires 1 input' % name)

        if mode == 'AND':
            spec_layer.logicalAnd.MergeFromString(b'')
        elif mode == 'OR':
            spec_layer.logicalOr.MergeFromString(b'')
        elif mode == 'XOR':
            spec_layer.logicalXor.MergeFromString(b'')
        elif mode == 'NOT':
            spec_layer.logicalNot.MergeFromString(b'')
        else:
            raise ValueError('Logical operation "%s" is not supported' % mode)

        return spec_layer

    def add_sliding_windows(self, name, input_name, output_name, axis, window_size, step=1):
        """
        Add a sliding_windows layer to the model that returns a tensor containing
        all windows of size ``window_size`` * separated by ``step`` along the dimension ``axis``.
        Refer to the **SlidingWindowsLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The of input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        axis: int
            Axis to perform the operation.
        window_size: int
            Number of elements in the sliding window.
        step: int, optional
            The stride of the input elements in the sliding window, default: 1.

        See Also
        --------
        add_slice, add_slice_static, add_slice_dynamic
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])

        spec_layer_params = spec_layer.slidingWindows
        spec_layer_params.axis = axis
        spec_layer_params.windowSize = window_size
        spec_layer_params.step = step

        self.rank_dict[output_name] = self._get_rank(input_name) + 1
        return spec_layer

    def add_reverse(self, name, input_name, output_name, reverse_dim=None):
        """
        Add a reverse layer to the model that reverses specific dimensions of
        the input tensor.
        Refer to the **ReverseLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        reverse_dim: list of int or tuple of int
            Reverse along the dimension, default [1].

        See Also
        --------
        add_reverse_sequence
        """

        if not reverse_dim:
            reverse_dim = [1]

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.reverse
        spec_layer_params.reverseDim.extend(map(bool, reverse_dim))
        return spec_layer

    def add_reverse_sequence(self, name, input_names, output_name,
                             batch_axis=0, seq_axis=-1):
        """
        Add a reverse sequence layer to the model that reverses variable length slices.
        Refer to the **ReverseSeqLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        batch_axis: int, optional
            Slices input along the dimension batch_axis, default 0.
        seq_axis: int, optional
            Reverse along the dimension seq_axis, default: -1.

        See Also
        --------
        add_reverse
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.reverseSeq.batchAxis = batch_axis
        spec_layer.reverseSeq.sequenceAxis = seq_axis

        return spec_layer

    def add_gather(self, name, input_names, output_name, axis=0):
        """
        Add a gather layer to the model that gathers elements or slices from
        data and store to a tensor whose shape is defined by indices from the
        input.
        Refer to the **GatherLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        axis: int, optional
            The axis the operation perform on, default: 0.

        See Also
        --------
        add_gather_nd, add_gather_along_axis, add_scatter, add_scatter_nd, add_scatter_along_axis
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.gather.axis = axis
        self.rank_dict[output_name] = self._get_rank(input_names[0]) - 1 + self._get_rank(input_names[1])
        return spec_layer

    def add_scatter(self, name, input_names, output_name, axis=0, mode='UPDATE'):
        """
        Add a scatter layer to the model that scatters data into a new tensor
        according to indices from the input.
        Refer to the **ScatterLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        axis: int
            The axis the operation perform on, default: 0.
        mode: str, optional
            Scatter accumulation mode in [UPDATE | ADD | SUB | MUL | DIV | MAX | MIN], default: UPDATE.

        See Also
        --------
        add_scatter_nd, add_scatter_along_axis, add_gather, add_gather_nd, add_gather_along_axis
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer_params = spec_layer.scatter
        spec_layer_params.axis = axis

        mode = mode.upper() if isinstance(mode, str) else mode
        if mode == 'UPDATE':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_UPDATE')
        elif mode == 'ADD':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_ADD')
        elif mode == 'SUB':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_SUB')
        elif mode == 'MUL':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_MUL')
        elif mode == 'DIV':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_DIV')
        elif mode == 'MAX':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_MAX')
        elif mode == 'MIN':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_MIN')
        else:
            raise ValueError("Unsupported Scatter mode %s" % mode)

        return spec_layer

    def add_gather_along_axis(self, name, input_names, output_name, axis=0):
        """
        Add a gather_along_axis layer to the model that gathers elements or slices
        from data and store to a tensor whose shape is defined by indices from the
        input along the given axis into the output tensor.
        Refer to the **GatherAlongAxisLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        axis: int, optional
            The axis the operation perform on, default: 0.

        See Also
        --------
        add_gather, add_gather_nd, add_scatter, add_scatter_nd, add_scatter_along_axis
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.gatherAlongAxis.axis = axis
        self.rank_dict[output_name] = self._get_rank(input_names[1])
        return spec_layer

    def add_scatter_along_axis(self, name, input_names, output_name, axis=0, mode='UPDATE'):
        """
        Add a scatter_along_axis layer to the model that scatters data into a new
        tensor according to indices from the input along the given axis into the
        output tensor.
        Refer to the **ScatterAlongAxisLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        axis: int
            The axis to perform on, default: 0.
        mode: str, optional
            Scatter accumulation mode in [UPDATE | ADD | SUB | MUL | DIV | MAX | MIN], default: UPDATE

        See Also
        --------
        add_scatter, add_scatter_nd, add_gather, add_gather_nd, add_gather_along_axis
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer_params = spec_layer.scatterAlongAxis
        spec_layer_params.axis = axis

        mode = mode.upper() if isinstance(mode, str) else mode
        if mode == 'UPDATE':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_UPDATE')
        elif mode == 'ADD':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_ADD')
        elif mode == 'SUB':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_SUB')
        elif mode == 'MUL':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_MUL')
        elif mode == 'DIV':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_DIV')
        elif mode == 'MAX':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_MAX')
        elif mode == 'MIN':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_MIN')
        else:
            raise ValueError('Unsupported scatter_along_axis mode %s' % mode)

        return spec_layer

    def add_gather_nd(self, name, input_names, output_name):
        """
        Add a gather layer to the model that gathers elements or slices from
        data and store to a tensor whose shape is defined by indices from the
        input. This is the reverse operation of the scatter operation.
        Refer to the **GatherNDLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_gather, add_gather_along_axis, add_scatter, add_scatter_nd, add_scatter_along_axis
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.gatherND.MergeFromString(b'')
        # NOTE: ideally, following is formula for computing output rank
        # self.rank_dict[output_name] = self._get_rank(input_names[1]) - 1 + self._get_rank(input_names[0])
        #                               + shape_dict[input_names[1]][-1]
        # But, shape of indices (input_names[1]) is unknown and hence marking as -1
        # Converter should update rank if indices are known
        self.rank_dict[output_name] = -1
        return spec_layer

    def add_scatter_nd(self, name, input_names, output_name, mode='UPDATE'):
        """
        Add a scatter layer to the model that scatters data into a new tensor
        according to indices from input. This is the reverse operation of the
        gather operation.
        Refer to the **ScatterNDLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        mode: str, optional
            Scatter accumulation mode in [UPDATE | ADD | SUB | MUL | DIV | MAX | MIN], default: UPDATE

        See Also
        --------
        add_scatter, add_scatter_along_axis, add_gather, add_gather_nd, add_gather_along_axis
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer_params = spec_layer.scatterND

        mode = mode.upper() if isinstance(mode, str) else mode
        if mode == 'UPDATE':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_UPDATE')
        elif mode == 'ADD':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_ADD')
        elif mode == 'SUB':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_SUB')
        elif mode == 'MUL':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_MUL')
        elif mode == 'DIV':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_DIV')
        elif mode == 'MAX':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_MAX')
        elif mode == 'MIN':
            spec_layer_params.mode = _NeuralNetwork_pb2.ScatterMode.Value('SCATTER_MIN')
        else:
            raise ValueError('Unsupported scatter mode %s' % mode)

        return spec_layer

    def add_topk(self, name, input_names, output_names, k=0, axis=0, use_bottom_k=False):
        """
        Add a topk layer to the model that returns top or bottom k values and
        the corresponding indices of the input tensor along a given axis.
        Refer to the **TopKLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer. It must be of length 1 or 2.
            The optional second input corresponds to value of K.
        output_names: list of str
            The output blob names of this layer. First and second correspond to
            values and indices, respectively.
        k: int, optional
            number of values/indices to be computed along the axis.
            Need not be given of there are two inputs, default: 0.
        axis: int, optional
            axis along which the topk values/indices are computed.
            negative indexing is supported, default: 0
        use_bottom_k: bool, optional
            if true, bottom k values are computed instead, default: false.
        """

        spec_layer = self._add_generic_layer(name, input_names, output_names)
        spec_layer_params = spec_layer.topK
        spec_layer_params.axis = axis
        spec_layer_params.K = k
        spec_layer_params.useBottomK = use_bottom_k
        return spec_layer

    def add_argmax(self, name, input_name, output_name, axis, keepdims=True):
        """
        Add an argmax layer to the model that returns the indices of the maximum
        value along a specified axis in the input tensor.
        Refer to the **ArgMaxLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        axis: int
            axis along which the argmax is computed. Negative indexing is supported.
        keepdims: bool, optional
            if true, output rank is same as input rank, default: true.

        See Also
        --------
        add_argmin
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.argMax
        spec_layer_params.axis = axis
        spec_layer_params.removeDim = not keepdims

        input_rank = self._get_rank(input_name)
        if input_rank == 1:
            self.rank_dict[output_name] = 1
        else:
            if keepdims:
                self.rank_dict[output_name] = input_rank
            else:
                self.rank_dict[output_name] = input_rank - 1
        return spec_layer

    def add_argmin(self, name, input_name, output_name, axis, keepdims=True):
        """
        Add an argmin layer to the model that returns the indices of the minimum
        value along a specified axis in the input tensor.
        Refer to the **ArgMinLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        axis: int
            axis along which the argmin is computed. Negative indexing is supported.
        keepdims: bool, optional
            if true, output rank is same as input rank, default: true.

        See Also
        --------
        add_argmax
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.argMin
        spec_layer_params.axis = axis
        spec_layer_params.removeDim = not keepdims

        input_rank = self._get_rank(input_name)
        if input_rank == 1:
            self.rank_dict[output_name] = 1
        else:
            if keepdims:
                self.rank_dict[output_name] = input_rank
            else:
                self.rank_dict[output_name] = input_rank - 1
        return spec_layer

    def add_constant_pad(self, name, input_names, output_name,
                         value=0.0, pad_to_given_output_size_mode=False, pad_amounts=[]):
        """
        Add a constant pad layer.
        Refer to the **ConstantPaddingLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob name(s) of this layer.
        output_name: str
            The output blob name of this layer.
        value: float
            value to be used for padding.
        pad_to_given_output_size_mode: bool
            if true, pad_amounts are interpreted as output shapes (see example in NeuralNetwork.proto)
        pad_amounts: [int], optional
            must be non negative. Amount to pad in each dimension. Length of the list must be twice the input/output rank.
            Not required if second input is present.

        See Also
        --------
        add_padding
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer_params = spec_layer.constantPad
        spec_layer_params.value = value
        spec_layer_params.padToGivenOutputSizeMode = pad_to_given_output_size_mode
        if len(pad_amounts) > 0:
            spec_layer_params.padAmounts.extend(map(int, pad_amounts))
        if len(input_names) == 1 and len(pad_amounts) == 0:
            raise ValueError("Constant_pad layer: pad_amounts must be provided when there is a single input")
        return spec_layer

    def add_nms(self, name, input_names, output_names,
                iou_threshold=0.5, score_threshold=0.0, max_boxes=1, per_class_suppression=False):
        """
        Add a non maximum suppression layer.
        Refer to the **NonMaximumSuppressionLayerParams** message in specification (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer. Must be at least 2, and maximum 5.
        output_names: list of str
            The output blob names of this layer. Must be of length 4 exactly.
        iou_threshold: float
            intersection over union threshold for suppression. Ignored if 3rd input is present.
        score_threshold: float
            threshold for selecting boxes to be used for NMS algorithm. Ignored if 4th input is present.
        max_boxes: int
            maximum number of boxes to output. Ignored if 5th input is present.
        per_class_suppression: bool
            If true, boxes are organized into classes and suppression is applied to each class group separately

        See Also
        --------
        add_constant_pad
        """

        spec_layer = self._add_generic_layer(name, input_names, output_names)
        spec_layer_params = spec_layer.NonMaximumSuppression
        spec_layer_params.iouThreshold = iou_threshold
        spec_layer_params.scoreThreshold = score_threshold
        spec_layer_params.maxBoxes = max_boxes
        spec_layer_params.perClassSuppression = per_class_suppression

        self.rank_dict[output_names[0]] = 3
        self.rank_dict[output_names[1]] = 3
        self.rank_dict[output_names[2]] = 2
        self.rank_dict[output_names[3]] = 1
        return spec_layer

    def add_embedding_nd(self, name, input_name, output_name,
                         vocab_size, embedding_size,
                         W, b=None,
                         is_quantized_weight=False,
                         quantization_type='linear',
                         nbits=8,
                         quant_scale=None,
                         quant_bias=None,
                         quant_lut=None):
        """
        Add an embedding layer to the model that performs a matrix lookup and
        optionally adds a bias.
        Refer to the **EmbeddingNDLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        vocab_size: int
            Size of the vocabulary (1 + maximum integer index of the words).
        embedding_size: int
            Size of the embedded vector.
        W: float32 numpy.array or bytes()
            Weight matrix of shape (embedding_size, vocab_size).
            If W is of type bytes(), i.e. quantized to 1-8 bits, other quantization
            related arguments must be provided as well (see below).
        b: numpy.array , optional
            Bias vector of shape (embedding_size, ).
        Quantization arguments expected, when W is of type bytes():
        is_quantized_weight: bool
            Set it to true when W is of type bytes(), representing quantized weights
        quantization_type: str
            When weights are quantized (i.e. W is of type bytes()), this should be either "linear" or "lut".
        nbits: int
            Should be between 1 and 8 (inclusive). Number of bits per weight value.
        quant_scale: numpy.array(dtype=numpy.float32)
            scale vector to be used with linear quantization. Must be of length either 1 or embedding_size.
        quant_bias: numpy.array(dtype=numpy.float32)
            bias vector to be used with linear quantization. Must be of length either 1 or embedding_size.
        quant_lut: numpy.array(dtype=numpy.float32)
            the LUT (look up table) to be used with LUT quantization. Must be of length 2^nbits.

        See Also
        --------
        add_inner_product, add_embedding
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])

        # Fill in the parameters
        spec_layer_params = spec_layer.embeddingND

        spec_layer_params.vocabSize = vocab_size
        spec_layer_params.embeddingSize = embedding_size
        spec_layer_params.hasBias = b is not None

        weights = spec_layer_params.weights
        if not is_quantized_weight:
            weights.floatValue.extend(map(float, W.flatten()))
        else:
            _verify_quantization_arguments(weight=W, output_channels=embedding_size,
                                           quantization_type=quantization_type, nbits=nbits,
                                           quant_scale=quant_scale, quant_bias=quant_bias, quant_lut=quant_lut)

            _fill_quantized_weights(weights_message=weights, W=W,
                                    quantization_type=quantization_type, nbits=nbits,
                                    quant_scale=quant_scale, quant_bias=quant_bias, quant_lut=quant_lut)

        if b is not None:
            bias = spec_layer_params.bias
            bias.floatValue.extend(map(float, b.flatten()))
        return spec_layer

    def add_batched_mat_mul(self, name, input_names, output_name,
                            transpose_a=False, transpose_b=False,
                            weight_matrix_rows=0, weight_matrix_columns=0,
                            W=None, bias=None,
                            is_quantized_weight=False,
                            quantization_type='linear',
                            nbits=8,
                            quant_scale=None,
                            quant_bias=None,
                            quant_lut=None):
        """
        Add a N-D Batched Matrix Multiplication layer with numpy like broadcasting.
        Refer to the **BatchedMatMulLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.

        input_names: list of str
            The input blob names of this layer.

        output_name: str
            The output blob name of this layer.

        transpose_a: bool, optional
            Whether or not to transpose A, default: false.

        transpose_b: bool, optional
            Whether or not to transpose B, default: false.

        weight_matrix_rows: int, optional
            Must be equal to the last dimension of the input, default: 0.

        weight_matrix_columns: int, optional
            Must be equal to the last dimension of the output, default: 0.

        W: float32 numpy.array or bytes(), optional
            Weight matrix of shape (weight_matrix_rows, weight_matrix_columns)
            If W is of type bytes(), i.e. quantized to 1-8 bits, other quantization
            related arguments must be provided as well (see below).

        bias: float32 numpy.array, optional
            Bias vector of shape (weight_matrix_columns,).

        Quantization arguments expected in kwargs, when W is of type bytes():

        is_quantized_weight: bool, optional
            Set it to true when W is of type bytes(), representing quantized weights, default: false.

        quantization_type: str, optional
            When weights are quantized (i.e. W is of type bytes()), this should be either "linear" or "lut", default: linear.

        nbits: int, optional
            Should be between 1 and 8 (inclusive). Number of bits per weight value, default: 8.

        quant_scale: numpy.array(dtype=numpy.float32), optional
            scale vector to be used with linear quantization. Must be of length either 1 or weight_matrix_columns, default: None.

        quant_bias: numpy.array(dtype=numpy.float32), optional
            bias vector to be used with linear quantization. Must be of length either 1 or weight_matrix_columns, default: None.

        quant_lut: numpy.array(dtype=numpy.float32), optional
            The LUT (look up table) to be used with LUT quantization. Must be of length 2^nbits, default: None.

        See Also
        --------
        add_inner_product
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])

        spec_layer_params = spec_layer.batchedMatmul
        spec_layer_params.transposeA = transpose_a
        spec_layer_params.transposeB = transpose_b

        if ((W is not None) or (bias is not None)) and len(input_names) == 2:
            raise ValueError("batched_mat_mul: Weight and/or bias are ignored when there are two inputs")

        if (W is None) and len(input_names) == 1:
            raise ValueError("batched_mat_mul: Weight parameter must be provided when there is one input")

        self.rank_dict[output_name] = 2
        for input_ in input_names:
            self.rank_dict[output_name] = max(self._get_rank(output_name), self._get_rank(input_))

        if len(input_names) == 1:
            spec_layer_params.weightMatrixFirstDimension = weight_matrix_rows
            spec_layer_params.weightMatrixSecondDimension = weight_matrix_columns
            spec_layer_params.hasBias = bias is not None

            weights = spec_layer_params.weights

            if not is_quantized_weight:
                weights.floatValue.extend(map(float, np.transpose(W).flatten()))
            else:
                _verify_quantization_arguments(weight=W, output_channels=weight_matrix_columns,
                                               quantization_type=quantization_type, nbits=nbits,
                                               quant_scale=quant_scale, quant_bias=quant_bias, quant_lut=quant_lut)

                num_weights = weight_matrix_rows * weight_matrix_columns
                if nbits < 8:
                    byte_arr = np.frombuffer(W, dtype=np.uint8)
                    W = unpack_to_bytes(byte_arr, num_weights, nbits)
                else:
                    W = np.frombuffer(W, dtype=np.uint8)

                W = np.reshape(W, (weight_matrix_rows, weight_matrix_columns))
                W = np.transpose(W)

                W_bytes = bytes()
                if nbits == 8:
                    W_bytes += W.flatten().tobytes()
                else:
                    W_bytes += _convert_array_to_nbit_quantized_bytes(W.flatten(), nbits).tobytes()

                _fill_quantized_weights(weights_message=weights, W=W_bytes,
                                        quantization_type=quantization_type, nbits=nbits,
                                        quant_scale=quant_scale, quant_bias=quant_bias, quant_lut=quant_lut)

            if bias is not None:
                bias_param = spec_layer_params.bias
                bias_param.floatValue.extend(map(float, bias.flatten()))

        return spec_layer

    def add_get_shape(self, name, input_name, output_name):
        """
        Add a get_shape layer to the model.
        Refer to the **GetShapeLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_reshape, add_reshape_like, add_reshape_static, add_reshape_dynamic
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.getShape.MergeFromString(b'')
        self.rank_dict[output_name] = 1
        return spec_layer

    def add_load_constant_nd(self, name, output_name, constant_value, shape):
        """
        Add a load_constant layer that loads data as a parameter and provides it
        as an output.
        Refer to the **LoadConstantNDLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        output_name: str
            The output blob name of this layer.
        constant_value: numpy.array()
            value of the constant as a numpy array.
        shape: list of int or tuple of int
            List of ints representing the shape of the constant.

        See Also
        --------
        add_elementwise
        """

        spec_layer = self._add_generic_layer(name, [], [output_name])
        spec_layer_params = spec_layer.loadConstantND

        data = spec_layer_params.data
        data.floatValue.extend(map(float, constant_value.flatten()))
        spec_layer_params.shape.extend(shape)

        # Rank information
        self.rank_dict[output_name] = len(shape)

        if len(data.floatValue) != np.prod(shape):
            raise ValueError("Dimensions of 'shape' do not match the size of the provided constant")
        return spec_layer

    def add_fill_like(self, name, input_name, output_name, value=0.):
        """
        Add a fill_like layer to the model outputs a tensor filled with a
        scalar value.
        Refer to the **FillLikeLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        value: float, optional
            A scalar value for the fill operation, default 0.

        See Also
        --------
        add_fill_static, add_fill_dynamic
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.fillLike
        spec_layer_params.value = value
        return spec_layer

    def add_fill_static(self, name, output_name, output_shape, value=0.):
        """
        Add a fill_static layer to the model that outputs a tensor filled
        with a scalar value given shape as parameter.
        Refer to the **FillStaticLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        output_name: str
            The output blob name of this layer.
        output_shape: list of int or tuple of int
            The target shape of the output tensor.
        value: float, optional
            A scalar value for the fill operation, default 0.

        See Also
        --------
        add_fill_like, add_fill_static
        """

        spec_layer = self._add_generic_layer(name, [], [output_name])
        spec_layer_params = spec_layer.fillStatic
        spec_layer_params.value = value
        spec_layer_params.targetShape.extend(output_shape)
        self.rank_dict[output_name] = len(output_shape)
        return spec_layer

    def add_fill_dynamic(self, name, input_name, output_name, value=0.):
        """
        Add a fill_dynamic layer to the model that outputs a tensor filled
        with a scalar value.
        Refer to the **FillDynamicLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        value: float, optional
            A scalar value for the fill operation, default: 0.

        See Also
        --------
        add_fill_like, add_fill_static
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.fillDynamic
        spec_layer_params.value = value
        self.rank_dict[output_name] = -1
        return spec_layer

    def add_broadcast_to_like(self, name, input_names, output_name):
        """
        Add a broadcast_to_like layer to the model that broadcasts a tensor
        to a compatible shape.
        Refer to the **BroadcastToLikeLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_broadcast_to_static, add_broadcast_to_dynamic
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.broadcastToLike.MergeFromString(b'')

        if len(input_names) != 2:
            raise ValueError("BroadcastToLikeLayer must have two inputs")

        self.rank_dict[output_name] = self._get_rank(input_names[1])
        return spec_layer

    def add_broadcast_to_static(self, name, input_name, output_name, output_shape):
        """
        Add a broadcast_to_static layer to the model that broadcasts a tensor
        to a compatible shape.
        Refer to the **BroadcastToStaticLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        output_shape: list of int or tuple of int
            The target shape of the output tensor.

        See Also
        --------
        add_broadcast_to_like, add_broadcast_to_dynamic
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.broadcastToStatic
        spec_layer_params.targetShape.extend(output_shape)

        self.rank_dict[output_name] = len(output_shape)
        return spec_layer

    def add_broadcast_to_dynamic(self, name, input_names, output_name):
        """
        Add a broadcast_to_dynamic layer to the model that broadcasts a tensor
        to a compatible shape.
        Refer to the **BroadcastToDynamicLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_broadcast_to_like, add_broadcast_to_static
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.broadcastToDynamic.MergeFromString(b'')
        # Setting rank to -1 is a hint that Rank was not computed
        # converter can modify if it's a constant and known
        self.rank_dict[output_name] = -1
        return spec_layer

    def add_expand_dims(self, name, input_name, output_name, axes):
        """
        Add an expand dims layer to the model that increases the rank of the
        input tensor by adding unit dimensions.
        Refer to the **ExpandDimsLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        axes: list of int or tuple of int
            Dimensions the operation perform on.

        See Also
        --------
        add_squeeze
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.expandDims
        spec_layer_params.axes.extend(axes)
        self.rank_dict[output_name] = self._get_rank(input_name) + len(axes)
        return spec_layer

    def add_squeeze(self, name, input_name, output_name, axes=None, squeeze_all=False):
        """
        Add a squeeze layer to the model that decrease the rank of the input
        tensor by removing unit dimensions.
        Refer to the **SqueezeLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        axes: list of int or tuple of int, optional
            Dimensions to perform the operation, default: None (squeeze_all).
        squeeze_all: bool, optional
            If true, all dimensions that are 1 are squeezed, default: false.

        See Also
        --------
        add_expand_dims
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.squeeze
        if axes is not None:
            spec_layer_params.axes.extend(axes)
        spec_layer_params.squeezeAll = squeeze_all

        if squeeze_all or axes is None:
            # All the dimensions that are 1 will be squeezed
            # converter should update rank if shape is known
            self.rank_dict[output_name] = -1
        else:
            rank = self._get_rank(input_name) - len(axes)
            self.rank_dict[output_name] = rank if rank != 0 else 1
        return spec_layer

    def add_flatten_to_2d(self, name, input_name, output_name, axis=1):
        """
        Add a flatten_to_2d layer to the model that flattens the input tensor
        into a 2-dimensional matrix.
        Refer to the **FlattenTo2DLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The of input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        axis: int, optional
            Axis to perform the operation, default: 1.

        See Also
        --------
        add_flatten
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.flattenTo2D
        spec_layer_params.axis = axis
        self.rank_dict[output_name] = 2
        return spec_layer

    def add_reshape_like(self, name, input_names, output_name):
        """
        Add a reshape_like layer to the model that reshapes a tensor.
        Refer to the **ReshapeLikeLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_reshape, add_reshape_static, add_reshape_dynamic, add_rank_preserving_reshape
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.reshapeLike.MergeFromString(b'')
        self.rank_dict[output_name] = self._get_rank(input_names[1])
        return spec_layer

    def add_reshape_static(self, name, input_name, output_name, output_shape):
        """
        Add a reshape_static layer to the model that reshapes a tensor.
        Refer to the **ReshapeStaticLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        output_shape: list of int or tuple of int
            Target shape of the output tensor.

        See Also
        --------
        add_reshape, add_reshape_like, add_reshape_dynamic, add_rank_preserving_reshape
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.reshapeStatic
        spec_layer_params.targetShape.extend(output_shape)
        self.rank_dict[output_name] = len(output_shape)
        return spec_layer

    def add_reshape_dynamic(self, name, input_names, output_name):
        """
        Add a reshape_dynamic layer to the model that reshapes a tensor.
        Refer to the **ReshapeDynamicLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_reshape, add_reshape_like, add_reshape_static, add_rank_preserving_reshape
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.reshapeDynamic.MergeFromString(b'')
        # Setting rank to -1 is a hint that Rank was not computed
        # converter can modify if it's a constant and known
        self.rank_dict[output_name] = -1
        return spec_layer

    def add_rank_preserving_reshape(self, name, input_name, output_name, output_shape):
        """
        Add a rank_preserving_reshape layer to the model that reshapes the input
        tensor without altering the rank of the tensor.
        Refer to the **RankPreservingReshapeLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        output_shape: list of int or tuple of int
            Determines the shape of the output blob.
            0: copy the dimension of the input to output
            -1: calculate dimensions from the rest of the shape

        See Also
        --------
        add_reshape, add_reshape_like, add_reshape_static, add_reshape_dynamic
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name],
                                             input_ranks=[len(output_shape)],
                                             input_shapes=[[int(x) for x in output_shape]],
                                             output_ranks=[len(output_shape)],
                                             output_shapes=[[int(x) for x in output_shape]])

        spec_layer_params = spec_layer.rankPreservingReshape
        spec_layer_params.targetShape.extend(map(int, output_shape))
        return spec_layer

    def add_random_normal_like(self, name, input_name, output_name, mean=0., stddev=0., seed=-1):
        """
        Add a random_normal_like layer to the model that fills the output
        tensor with random values from normal distribution.
        Refer to the **RandomNormalLikeLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        mean: float, optional
            The mean of the normal distribution, default: 0.0.
        stddev: float, optional
            The standard deviation of the normal distribution, default: 1.0.
        seed: int, optional
            Used to create a random seed for the distribution, default -1 (random).

        See Also
        --------
        add_random_normal_static, add_random_normal_dynamic
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.randomNormalLike

        spec_layer_params.mean = mean
        spec_layer_params.stdDev = stddev
        spec_layer_params.seed = seed

        return spec_layer

    def add_random_normal_static(self, name, output_name, output_shape, mean=0., stddev=0., seed=-1):
        """
        Add a random_normal_static layer to the model that fills the output
        tensor with random values from normal distribution.
        Refer to the **RandomNormaStaticLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        output_name: str
            The output blob name of this layer.
        output_shape: list of int or tuple of int
            Target shape of the output tensor.
        mean: float, optional
            The mean of the normal distribution, default: 0.0.
        stddev: float, optional
            The standard deviation of the normal distribution, default: 1.0.
        seed: int, optional
            Used to create a random seed for the distribution. Default -1 (random).

        See Also
        --------
        add_random_normal_like, add_random_normal_dynamic
        """

        spec_layer = self._add_generic_layer(name, [], [output_name])
        spec_layer_params = spec_layer.randomNormalStatic

        spec_layer_params.outputShape.extend(output_shape)
        spec_layer_params.mean = mean
        spec_layer_params.stdDev = stddev
        spec_layer_params.seed = seed

        self.rank_dict[output_name] = len(output_shape)
        return spec_layer

    def add_random_normal_dynamic(self, name, input_names, output_name, mean=0., stddev=0., seed=-1):
        """
        Add a random_normal_dynamic layer to the model that fills the output
        tensor with random values from normal distribution.
        Refer to the **RandomNormalDynamicLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        mean: float, optional
            The mean of the normal distribution, default: 0.0.
        stddev: float, optional
            The standard deviation of the normal distribution, default: 1.0.
        seed: int, optional
            Used to create a random seed for the distribution. Default -1 (random).

        See Also
        --------
        add_random_normal_like, add_random_normal_static
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer_params = spec_layer.randomNormalDynamic

        spec_layer_params.mean = mean
        spec_layer_params.stdDev = stddev
        spec_layer_params.seed = seed
        # Setting rank to -1 is a hint that Rank was not computed
        # converter can modify if it's a constant and known
        self.rank_dict[output_name] = -1
        return spec_layer

    def add_random_uniform_like(self, name, input_name, output_name, minval=0., maxval=1., seed=-1):
        """
        Add a random_uniform_like layer to the model that fills the output
        tensors with random values from uniform distribution.
        Refer to the **RandomUniformLikeLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        minval: float, optional
            Lower bound / minimum value of the uniform distribution, default: 0.0.
        maxval: float, optional
            Upper bound / maximum value of the uniform distribution, default: 1.0.
        seed: int, optional
            Used to create a random seed for the distribution. default -1 (random).

        See Also
        --------
        add_random_uniform_static, add_random_uniform_dynamic
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.randomUniformLike

        spec_layer_params.minVal = minval
        spec_layer_params.maxVal = maxval
        spec_layer_params.seed = seed

        return spec_layer

    def add_random_uniform_static(self, name, output_name, output_shape, minval=0., maxval=1., seed=-1):
        """
        Add a random_uniform_static layer to the model that fills the output
        tensors with random values from uniform distribution.
        Refer to the **RandomUniformStaticLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        output_name: str
            The output blob name of this layer.
        output_shape: list of int or tuple of int
            Target shape of the output tensor.
        minval: float, optional
            Lower bound / minimum value of the uniform distribution, default: 0.0.
        maxval: float, optional
            Upper bound / maximum value of the uniform distribution, default: 1.0.
        seed: int, optional
            Used to create a random seed for the distribution. default -1 (random).

        See Also
        --------
        add_random_uniform_like, add_random_uniform_dynamic
        """

        spec_layer = self._add_generic_layer(name, [], [output_name])
        spec_layer_params = spec_layer.randomUniformStatic

        spec_layer_params.outputShape.extend(output_shape)
        spec_layer_params.minVal = minval
        spec_layer_params.maxVal = maxval
        spec_layer_params.seed = seed
        self.rank_dict[output_name] = len(output_shape)
        return spec_layer

    def add_random_uniform_dynamic(self, name, input_names, output_name, minval=0., maxval=1., seed=-1):
        """
        Add a random_uniform_dynamic layer to the model that fills the output
        tensors with random values from uniform distribution.
        Refer to the **RandomUniformDynamicLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        minval: float, optional
            Lower bound / minimum value of the uniform distribution, default: 0.0.
        maxval: float, optional
            Upper bound / maximum value of the uniform distribution, default: 1.0.
        seed: int, optional
            Used to create a random seed for the distribution. default -1 (random).

        See Also
        --------
        add_random_uniform_like, add_random_uniform_static
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer_params = spec_layer.randomUniformDynamic

        spec_layer_params.minVal = minval
        spec_layer_params.maxVal = maxval
        spec_layer_params.seed = seed
        # Setting rank to -1 is a hint that Rank was not computed
        # converter can modify if it's a constant and known
        self.rank_dict[output_name] = -1
        return spec_layer

    def add_random_bernoulli_like(self, name, input_name, output_name, prob=0.5, seed=-1):
        """
        Add a random_bernoulli_like layer to the model that fills the output
        tensor with random values from Bernoulli distribution.
        Refer to the **RandomBernoulliLikeLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        prob: float, optional
            Probabilities for Bernoulli distribution, default: 0.5.
        seed: int, optional
            Used to create a random seed for the distribution. default -1 (random).

        See Also
        --------
        add_random_bernoulli_static, add_random_bernoulli_dynamic
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.randomBernoulliLike

        spec_layer_params.prob = prob
        spec_layer_params.seed = seed

        return spec_layer

    def add_random_bernoulli_static(self, name, output_name, output_shape, prob=0.5, seed=-1):
        """
        Add a random_bernoulli_static layer to the model that fills the output
        tensor with random values from Bernoulli distribution.
        Refer to the **RandomBernoulliStaticLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        output_name: str
            The output blob name of this layer.
        output_shape: list of int or tuple of int
            Target shape of the output tensor.
        prob: float, optional
            Probabilities for Bernoulli distribution, default: 0.5.
        seed: int, optional
            Used to create a random seed for the distribution. default -1 (random).

        See Also
        --------
        add_random_bernoulli_like, add_random_bernoulli_dynamic
        """

        spec_layer = self._add_generic_layer(name, [], [output_name])
        spec_layer_params = spec_layer.randomBernoulliStatic

        spec_layer_params.outputShape.extend(output_shape)
        spec_layer_params.prob = prob
        spec_layer_params.seed = seed

        self.rank_dict[output_name] = len(output_shape)
        return spec_layer

    def add_random_bernoulli_dynamic(self, name, input_names, output_name, prob=0.5, seed=-1):
        """
        Add a random_bernoulli_dynamic layer to the model that fills the output
        tensor with random values from Bernoulli distribution.
        Refer to the **RandomBernoulliDynamicLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.
        prob: float, optional
            Probabilities for Bernoulli distribution, default: 0.5.
        seed: int, optional
            Used to create a random seed for the distribution. default -1 (random).

        See Also
        --------
        add_random_bernoulli_like, add_random_bernoulli_static
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer_params = spec_layer.randomBernoulliDynamic

        spec_layer_params.prob = prob
        spec_layer_params.seed = seed

        # Setting rank to -1 is a hint that Rank was not computed
        # converter can modify if it's a constant and known
        self.rank_dict[output_name] = -1
        return spec_layer

    def add_categorical_distribution(self, name, input_name, output_name, num_samples,
                                     is_logits=True, eps=1e-10, temperature=1.0, seed=-1):
        """
        Add a categorical_distribution layer to the model that fills the output
        tensor with random values from categorical distribution.
        Refer to the **CategoricalDistributionLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        num_samples: int
            List of dimensions for the reduce operations.
        is_logits: bool, optional
            If true, the input is log probabilities. If false, the input is
            probabilities, default: True
        eps: float, optional
            Epsilon parameter for categorical distribution, default 1e-10.
        temperature: float, optional
            Temperature parameter for categorical distribution, default 1.0.
        seed: int, optional
            Used to create a random seed for the distribution. default -1 (random).
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.categoricalDistribution

        spec_layer_params.numSamples = num_samples
        spec_layer_params.isLogits = is_logits
        spec_layer_params.eps = eps
        spec_layer_params.temperature = temperature
        spec_layer_params.seed = seed

        return spec_layer

    def add_reduce_sum(self, name, input_name, output_name,
                       axes=None, keepdims=True, reduce_all=False):
        """
        Add a reduce_sum layer to the model that reduces the input tensor
        using ``sum(elements across given dimensions)``.
        Refer to the **ReduceSumLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        axes: list of int or tuple of int, optional
            List of dimensions for the reduce operations.
            Each should be in range [-rank(input), rank(input)), default: None (reduce_all)
        keepdims: bool, optional
            Whether or not to retain the reduced dimensions with length 1, default: true.
        reduce_all: bool, optional
            Whether or not to reduce on all axes, default: false.

        See Also
        --------
        add_reduce_l1, add_reduce_l2, add_reduce_min, add_reduce_prod,
        add_reduce_max, add_reduce_mean, add_reduce_logsum, add_reduce_logsumexp,
        add_reduce_sumsquare
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.reduceSum

        if axes and len(axes) != 0:
            spec_layer_params.axes.extend(map(int, axes))
        else:
            reduce_all = True

        spec_layer_params.keepDims = keepdims
        spec_layer_params.reduceAll = reduce_all

        self._set_rank_for_reduce_op(input_name, output_name, axes, keepdims, reduce_all)
        return spec_layer

    def add_reduce_prod(self, name, input_name, output_name,
                        axes=None, keepdims=True, reduce_all=False):
        """
        Add a reduce_prod layer to the model that reduces the input tensor
        using ``prod(elements across given dimensions)``.
        Refer to the **ReduceProdLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        axes: list of int or tuple of int, optional
            List of dimensions for the reduce operations.
            Each should be in range [-rank(input), rank(input)), default: None (reduce_all)
        keepdims: bool, optional
            Whether or not to retain the reduced dimensions with length 1, default: true.
        reduce_all: bool, optional
            Whether or not to reduce on all axes. If axes list is empty, it will
            be set to true, default: false.

        See Also
        --------
        add_reduce_l1, add_reduce_l2, add_reduce_sum, add_reduce_min,
        add_reduce_max, add_reduce_mean, add_reduce_logsum, add_reduce_logsumexp,
        add_reduce_sumsquare
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.reduceProd

        if axes and len(axes) != 0:
            spec_layer_params.axes.extend(map(int, axes))
        else:
            reduce_all = True

        spec_layer_params.keepDims = keepdims
        spec_layer_params.reduceAll = reduce_all

        self._set_rank_for_reduce_op(input_name, output_name, axes, keepdims, reduce_all)
        return spec_layer

    def add_reduce_mean(self, name, input_name, output_name,
                        axes=None, keepdims=True, reduce_all=False):
        """
        Add a reduce_mean layer to the model that reduces the input tensor
        using ``mean(elements across given dimensions)``.
        Refer to the **ReduceMeanLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        axes: list of int or tuple of int, optional
            List of dimensions for the reduce operations.
            Each should be in range [-rank(input), rank(input)), default: None (reduce_all)
        keepdims: bool, optional
            Whether or not to retain the reduced dimensions with length 1, default: true.
        reduce_all: bool, optional
            Whether or not to reduce on all axes, default: false.

        See Also
        --------
        add_reduce_l1, add_reduce_l2, add_reduce_sum, add_reduce_min, add_reduce_prod
        add_reduce_max, add_reduce_logsum, add_reduce_logsumexp, add_reduce_sumsquare
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.reduceMean

        if axes and len(axes) != 0:
            spec_layer_params.axes.extend(map(int, axes))
        else:
            reduce_all = True

        spec_layer_params.keepDims = keepdims
        spec_layer_params.reduceAll = reduce_all

        self._set_rank_for_reduce_op(input_name, output_name, axes, keepdims, reduce_all)
        return spec_layer

    def add_reduce_max(self, name, input_name, output_name,
                       axes=None, keepdims=True, reduce_all=False):
        """
        Add a reduce_max layer to the model that reduces the input tensor
        using ``max(elements across given dimensions)``.
        Refer to the **ReduceMaxLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        axes: list of int or tuple of int, optional
            List of dimensions for the reduce operations.
            Each should be in range [-rank(input), rank(input)), default: None (reduce_all)
        keepdims: bool, optional
            Whether or not to retain the reduced dimensions with length 1, default: true.
        reduce_all: bool, optional
            Whether or not to reduce on all axes, default: false.

        See Also
        --------
        add_reduce_l1, add_reduce_l2, add_reduce_sum, add_reduce_min, add_reduce_prod
        add_reduce_mean, add_reduce_logsum, add_reduce_logsumexp, add_reduce_sumsquare
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.reduceMax

        if axes and len(axes) != 0:
            spec_layer_params.axes.extend(map(int, axes))
        else:
            reduce_all = True

        spec_layer_params.keepDims = keepdims
        spec_layer_params.reduceAll = reduce_all

        self._set_rank_for_reduce_op(input_name, output_name, axes, keepdims, reduce_all)
        return spec_layer

    def add_reduce_min(self, name, input_name, output_name,
                       axes=None, keepdims=True, reduce_all=False):
        """
        Add a reduce_min layer to the model that reduces the input tensor
        using ``min(elements across given dimensions)``.
        Refer to the **ReduceMinLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        axes: list of int or tuple of int, optional
            List of dimensions for the reduce operations.
            Each should be in range [-rank(input), rank(input)), default: None (reduce_all)
        keepdims: bool, optional
            Whether or not to retain the reduced dimensions with length 1, default: true.
        reduce_all: bool, optional
            Whether or not to reduce on all axes, default: false.

        See Also
        --------
        add_reduce_l1, add_reduce_l2, add_reduce_sum, add_reduce_max, add_reduce_prod
        add_reduce_mean, add_reduce_logsum, add_reduce_logsumexp, add_reduce_sumsquare
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.reduceMin

        if axes and len(axes) != 0:
            spec_layer_params.axes.extend(map(int, axes))
        else:
            reduce_all = True

        spec_layer_params.keepDims = keepdims
        spec_layer_params.reduceAll = reduce_all

        self._set_rank_for_reduce_op(input_name, output_name, axes, keepdims, reduce_all)
        return spec_layer

    def add_reduce_l2(self, name, input_name, output_name,
                      axes=None, keepdims=True, reduce_all=False):
        """
        Add a reduce_l2 layer to the model that reduces the input tensor
        using ``l2_normalization(elements across given dimensions)``.
        Refer to the **ReduceL2LayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        axes: list of int or tuple of int, optional
            List of dimensions for the reduce operations.
            Each should be in range [-rank(input), rank(input)), default: None (reduce_all)
        keepdims: bool, optional
            Whether or not to retain the reduced dimensions with length 1, default: true.
        reduce_all: bool, optional
            Whether or not to reduce on all axes, default: false.

        See Also
        --------
        add_reduce_l1, add_reduce_sum, add_reduce_min, add_reduce_max, add_reduce_prod
        add_reduce_mean, add_reduce_logsum, add_reduce_logsumexp, add_reduce_sumsquare
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.reduceL2

        if axes and len(axes) != 0:
            spec_layer_params.axes.extend(map(int, axes))
        else:
            reduce_all = True

        spec_layer_params.keepDims = keepdims
        spec_layer_params.reduceAll = reduce_all

        self._set_rank_for_reduce_op(input_name, output_name, axes, keepdims, reduce_all)
        return spec_layer

    def add_reduce_l1(self, name, input_name, output_name,
                      axes=None, keepdims=True, reduce_all=False):
        """
        Add a reduce_l1 layer to the model that reduces the input tensor
        using ``l1_normalization(elements across given dimensions)``.
        Refer to the **ReduceL1LayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        axes: list of int or tuple of int, optional
            List of dimensions for the reduce operations.
            Each should be in range [-rank(input), rank(input)), default: None (reduce_all)
        keepdims: bool, optional
            Whether or not to retain the reduced dimensions with length 1, default: true.
        reduce_all: bool, optional
            Whether or not to reduce on all axes, default: false.

        See Also
        --------
        add_reduce_l2, add_reduce_sum, add_reduce_min, add_reduce_max, add_reduce_prod
        add_reduce_mean, add_reduce_logsum, add_reduce_logsumexp, add_reduce_sumsquare
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.reduceL1

        if axes and len(axes) != 0:
            spec_layer_params.axes.extend(map(int, axes))
        else:
            reduce_all = True

        spec_layer_params.keepDims = keepdims
        spec_layer_params.reduceAll = reduce_all

        self._set_rank_for_reduce_op(input_name, output_name, axes, keepdims, reduce_all)
        return spec_layer

    def add_reduce_sumsquare(self, name, input_name, output_name,
                             axes=None, keepdims=True, reduce_all=False):
        """
        Add a reduce_sumsquare layer to the model that reduces the input tensor
        using ``sum(square(elements across given dimensions))``.
        Refer to the **ReduceSumSquareLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        axes: list of int or tuple of int, optional
            List of dimensions for the reduce operations.
            Each should be in range [-rank(input), rank(input)), default: None (reduce_all)
        keepdims: bool, optional
            Whether or not to retain the reduced dimensions with length 1, default: true.
        reduce_all: bool, optional
            Whether or not to reduce on all axes, default: false.

        See Also
        --------
        add_reduce_l1, add_reduce_l2, add_reduce_sum, add_reduce_min, add_reduce_prod
        add_reduce_max, add_reduce_mean, add_reduce_logsum, add_reduce_logsumexp
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.reduceSumSquare

        if axes and len(axes) != 0:
            spec_layer_params.axes.extend(map(int, axes))
        else:
            reduce_all = True

        spec_layer_params.keepDims = keepdims
        spec_layer_params.reduceAll = reduce_all

        self._set_rank_for_reduce_op(input_name, output_name, axes, keepdims, reduce_all)
        return spec_layer

    def add_reduce_logsum(self, name, input_name, output_name,
                          axes=None, keepdims=True, reduce_all=False):
        """
        Add a reduce_logsum layer to the model that reduces the input tensor
        using log(sum(elements across given dimensions)).
        Refer to the **ReduceLogSumLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        axes: list of int or tuple of int, optional
            List of dimensions for the reduce operations.
            Each should be in range [-rank(input), rank(input)), default: None (reduce_all)
        keepdims: bool, optional
            Whether or not to retain the reduced dimensions with length 1, default: true.
        reduce_all: bool, optional
            Whether or not to reduce on all axes, default: false.

        See Also
        --------
        add_reduce_l1, add_reduce_l2, add_reduce_sum, add_reduce_min, add_reduce_prod
        add_reduce_max, add_reduce_mean, add_reduce_logsumexp, add_reduce_sumsquare
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.reduceLogSum

        if axes and len(axes) != 0:
            spec_layer_params.axes.extend(map(int, axes))
        else:
            reduce_all = True

        spec_layer_params.keepDims = keepdims
        spec_layer_params.reduceAll = reduce_all

        self._set_rank_for_reduce_op(input_name, output_name, axes, keepdims, reduce_all)
        return spec_layer

    def add_reduce_logsumexp(self, name, input_name, output_name,
                             axes=None, keepdims=True, reduce_all=False):
        """
        Add a reduce_logsumexp layer to the model that computes ``log(sum(exp(tensor)))``
        and reduces along the given axis.
        Refer to the **ReduceLogSumExpLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        axes: list of int or tuple of int, optional
            List of dimensions for the reduce operations.
            Each should be in range [-rank(input), rank(input)), default: None (reduce_all)
        keepdims: bool, optional
            Whether or not to retain the reduced dimensions with length 1, default: true.
        reduce_all: bool, optional
            Whether or not to reduce on all axes, default: false.

        See Also
        --------
        add_reduce_l1, add_reduce_l2, add_reduce_sum, add_reduce_min, add_reduce_prod
        add_reduce_max, add_reduce_mean, add_reduce_logsum, add_reduce_sumsquare
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.reduceLogSumExp

        if axes and len(axes) != 0:
            spec_layer_params.axes.extend(map(int, axes))
        else:
            reduce_all = True

        spec_layer_params.keepDims = keepdims
        spec_layer_params.reduceAll = reduce_all

        self._set_rank_for_reduce_op(input_name, output_name, axes, keepdims, reduce_all)
        return spec_layer

    def add_where_nonzero(self, name, input_name, output_name):
        """
        Add a where_nonzero layer to the model that returns a tensor containing
        the indices of all non-zero elements of input tensor.
        Refer to the **WhereNonZeroLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_where_broadcastable
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer.whereNonZero.MergeFromString(b'')

        self.rank_dict[output_name] = 2
        return spec_layer

    def add_matrix_band_part(self, name, input_name, output_name, num_lower=-1, num_upper=-1):
        """
        Add a matrix_band_part layer to the model that copies a tensor setting
        everything outside a central band in each inner-most matrix to zero.
        Refer to the **MatrixBandPartLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The of input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        num_lower: int, optional
            Number of lower sub-diagonals to keep.
            Default: -1 (keep entire lower triangle).
        num_upper: int, optional
            Number of upper sub-diagonals to keep.
            Default: -1 (keep entire upper triangle).

        See Also
        --------
        add_lower_triangular, add_lower_triangular
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.matrixBandPart
        spec_layer_params.numLower = num_lower
        spec_layer_params.numUpper = num_upper
        return spec_layer

    def add_lower_triangular(self, name, input_name, output_name, k=0):
        """
        Add a lower_triangular layer to the model that copies a tensor setting
        everything outside lower triangular to zero.
        Refer to the **LowerTriangularLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The of input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        k: int, optional
            Diagonal below which to zero elements, default: 0 (main diagonal),
            k < 0 is lower it and k > 0 is upper.

        See Also
        --------
        add_upper_triangular, add_matrix_band_part
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.lowerTriangular
        spec_layer_params.k = k
        return spec_layer

    def add_upper_triangular(self, name, input_name, output_name, k=0):
        """
        Add a upper_triangular layer to the model that copies a tensor setting
        everything outside upper triangular to zero.
        Refer to the **UpperTriangularLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The of input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        k: int, optional
            Diagonal above which to zero elements, default: 0 (main diagonal),
            k < 0 is lower it and k > 0 is upper.

        See Also
        --------
        add_lower_triangular, add_matrix_band_part
        """

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.upperTriangular
        spec_layer_params.k = k
        return spec_layer

    def add_where_broadcastable(self, name, input_names, output_name):
        """
        Add a where_broadcastable layer to the model that returns the elements
        either from tensor x or tensor y, depending on the value in the
        condition tensor.
        Refer to the **WhereBroadcastableLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_names: list of str
            The input blob names of this layer.
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_where_nonzero
        """

        spec_layer = self._add_generic_layer(name, input_names, [output_name])
        spec_layer.whereBroadcastable.MergeFromString(b'')

        self._set_max_input_rank(input_names, output_name)

        return spec_layer

    def add_layer_normalization(self, name, input_name, output_name,
                                normalized_shape, gamma, beta, eps=1e-5):
        """
        Add a layer normalization layer to the model that applies layer
        normalization over the input tensor.
        Refer to the **LayerNormalizationLayerParams** message in specification
        (NeuralNetwork.proto) for more details.

        Parameters
        ----------
        name: str
            The name of this layer.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        normalized_shape: list of int or tuple of int
            Input shape from an expected input of size.
        gamma: WeightParams
            Weight parameters.
        beta: WeightParams
            Bias parameters.
        eps: float, optional
            Constant value added to the denominator, default: 1e-5.
        """

        if gamma.shape != tuple(normalized_shape):
            raise ValueError('Shape of parameter gamma should match normalized_shape')

        if beta.shape != tuple(normalized_shape):
            raise ValueError('Shape of parameter beta should match normalized_shape')

        spec_layer = self._add_generic_layer(name, [input_name], [output_name])
        spec_layer_params = spec_layer.layerNormalization

        spec_layer_params.normalizedShape.extend(normalized_shape)

        weights = spec_layer_params.gamma
        weights.floatValue.extend(map(float, gamma.flatten()))

        bias = spec_layer_params.beta
        bias.floatValue.extend(map(float, beta.flatten()))

        spec_layer_params.eps = eps

        return spec_layer
