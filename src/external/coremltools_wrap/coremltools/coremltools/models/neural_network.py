# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

"""
Neural network builder class to construct Core ML models.
"""
from .. import SPECIFICATION_VERSION
from ..proto import Model_pb2 as _Model_pb2
from ..proto import NeuralNetwork_pb2 as _NeuralNetwork_pb2
from ..proto import FeatureTypes_pb2 as _FeatureTypes_pb2
from ._interface_management import set_transform_interface_params
from . import datatypes
import numpy as np


def _set_recurrent_activation(param, activation):
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

class NeuralNetworkBuilder(object):
    """
    Neural network builder class to construct Core ML models.

    The NeuralNetworkBuilder constructs a Core ML neural network specification
    layer by layer. The layers should be added in such an order that the inputs
    to each layer (referred to as blobs) of each layer has been previously
    defined.  The builder can also set pre-processing steps to handle
    specialized input format (e.g. images), and set class labels for neural
    network classifiers.

    Please see the Core ML neural network protobuf message for more information
    on neural network layers, blobs, and parameters.

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
        >>> builder.add_inner_product(name = 'ip_layer', W = weights, b = bias, input_channels = 3, output_channels = 2,
        ... has_bias = True, input_name = 'data', output_name = 'probs')

        # save the spec by the builder
        >>> save_spec(builder.spec, 'network.mlmodel')

    See Also
    --------
    MLModel, datatypes, save_spec
    """

    def __init__(self, input_features, output_features, mode = None):
        """
        Construct a NeuralNetworkBuilder object and set protobuf specification interface.

        Parameters
        ----------
        input_features: [(str, datatypes.Array)]
            List of input feature of the network. Each feature is a (name,
            array) tuple, where name the name of the feature, and array
            is an datatypes.Array object describing the feature type.

        output_features: [(str, datatypes.Array or None)]
            List of output feature of the network. Each feature is a (name,
            array) tuple, where name is the name of the feature, and array
            is an datatypes.Array object describing the feature type.
            array can be None if not known.

        mode: str ('classifier', 'regressor' or None)
            Mode (one of 'classifier', 'regressor', or None).

            When mode = 'classifier', a NeuralNetworkClassifier spec will be
            constructed.  When mode = 'regressor', a NeuralNetworkRegressor
            spec will be constructed.

        Examples
        --------
        .. sourcecode:: python

            # Construct a builder that builds a neural network classifier with a 299x299x3
            # dimensional input and 1000 dimensional output
            >>> input_features = [('data', datatypes.Array((299,299,3)))]
            >>> output_features = [('probs', datatypes.Array((1000,)))]
            >>> builder = NeuralNetworkBuilder(input_features, output_features, mode='classifier')

        See Also
        --------
        set_input, set_output, set_class_labels
        """
        # Set the interface params.
        spec = _Model_pb2.Model()
        spec.specificationVersion = SPECIFICATION_VERSION

        in_features = input_features
        out_features = []
        unk_shape_indices = []
        for idx, feature in enumerate(output_features):
            name, arr = feature
            if arr is None:
                arr = datatypes.Array(1)
                unk_shape_indices.append(idx)
            out_features.append((str(name), arr))

        # When output_features in None, use some dummy sized type
        out_features_with_shape = []
        for out_feature in output_features:
            feat_name, feat_type = out_feature
            if feat_type is None:
                out_features_with_shape.append((feat_name, datatypes.Array(1)))
            else:
                out_features_with_shape.append(out_feature)

        # Set interface inputs and outputs
        # set_transform_interface_params require output types and shapes,
        # so we call it then remove the shape
        spec = set_transform_interface_params(spec, input_features,
                                              out_features_with_shape)

        for idx, output_feature in enumerate(output_features):
            if output_features[idx][1] is None:
                spec.description.output[idx].type.multiArrayType.ClearField(
                        "shape")

        # Save the spec in the protobuf
        self.spec = spec
        if mode == 'classifier':
            nn_spec = spec.neuralNetworkClassifier
        elif mode == 'regressor':
            nn_spec = spec.neuralNetworkRegressor
        else:
            nn_spec = spec.neuralNetwork
        self.nn_spec = nn_spec

    def set_input(self, input_names, input_dims):
        """
        Set the inputs of the network spec.

        Parameters
        ----------
        input_names: [str]
            List of input names of the network.

        input_dims: [tuple]
            List of input dimensions of the network. The ordering of input_dims
            is the same as input_names.

        Examples
        --------
        .. sourcecode:: python

            # Set the neural network spec inputs to be 3 dimensional vector data1 and
            # 4 dimensional vector data2.
            >>> builder.set_input(input_names = ['data1', 'data2'], [(3,), (4,)])

        See Also
        --------
        set_output, set_class_labels
        """
        spec = self.spec
        nn_spec = self.nn_spec
        for idx, dim in enumerate(input_dims):
            if len(dim) == 3:
                input_shape = (dim[0], dim[1], dim[2])
            elif len(dim) == 2:
                input_shape = (dim[1], )
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

    def set_output(self, output_names, output_dims):
        """
        Set the outputs of the network spec.

        Parameters
        ----------
        output_names: [str]
            List of output names of the network.

        output_dims: [tuple]
            List of output dimensions of the network. The ordering of output_dims is the same
            as output_names.

        Examples
        --------
        .. sourcecode:: python

            # Set the neural network spec outputs to be 3 dimensional vector feature1 and
            # 4 dimensional vector feature2.
            >>> builder.set_output(output_names = ['feature1', 'feature2'], [(3,), (4,)])

        See Also
        --------
        set_input, set_class_labels
        """
        spec = self.spec
        nn_spec = self.nn_spec
        for idx, dim in enumerate(output_dims):
            spec.description.output[idx].type.multiArrayType.ClearField("shape")
            spec.description.output[idx].type.multiArrayType.shape.extend(dim)
            spec.description.output[idx].type.multiArrayType.dataType = \
                    _Model_pb2.ArrayFeatureType.DOUBLE

    def set_class_labels(self, class_labels, predicted_feature_name = 'classLabel', prediction_blob = ''):
        """
        Set class labels to the model spec to make it a neural network classifier.

        Parameters
        ----------
        class_labels: list[int or str]
            A list of integers or strings that map the index of the output of a
            neural network to labels in a classifier.

        predicted_feature_name: str
            Name of the output feature for the class labels exposed in the
            Core ML neural network classifier.  Defaults to 'class_output'.

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
        else: #not provided
            # assume it's the last blob produced in the network
            nn_spec.labelProbabilityLayerName = nn_spec.layers[-1].output[0]


    def add_optionals(self, optionals_in, optionals_out):
        """
        Add optional inputs and outputs to the model spec.

        Parameters
        ----------
        optionals_in: [str]
            List of inputs that are optionals.

        optionals_out: [str]
            List of outputs that are optionals.

        See Also
        --------
        set_input, set_output

        """
        spec = self.spec
        if (not optionals_in) and (not optionals_out):
            return

        # assuming single sizes here
        input_types = [datatypes.Array(dim) for (name, dim) in optionals_in]
        output_types = [datatypes.Array(dim) for (name, dim) in optionals_out]

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


    def add_inner_product(self, name, W, b, input_channels, output_channels, has_bias,
                          input_name, output_name):
        """
        Add an inner product layer to the model.

        Parameters
        ----------
        name: str
            The name of this layer
        W: numpy.array
            Weight matrix of shape (output_channels, input_channels).
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

        See Also
        --------
        add_embedding, add_convolution
        """

        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)
        spec_layer_params = spec_layer.innerProduct

        # Fill in the parameters
        spec_layer_params.inputChannels = input_channels
        spec_layer_params.outputChannels = output_channels
        spec_layer_params.hasBias = has_bias

        weights = spec_layer_params.weights
        weights.floatValue.extend(map(float, W.flatten()))
        if has_bias:
            bias = spec_layer_params.bias
            bias.floatValue.extend(map(float, b.flatten()))

    def add_embedding(self, name, W, b, input_dim, output_channels, has_bias,
                      input_name, output_name):
        """
        Add an embedding layer to the model.

        Parameters
        ----------
        name: str
            The name of this layer
        W: numpy.array
            Weight matrix of shape (output_channels, input_dim).
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

        See Also
        --------
        add_inner_product
        """

        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)

        # Fill in the parameters
        spec_layer_params = spec_layer.embedding

        spec_layer_params.inputDim = input_dim
        spec_layer_params.outputChannels = output_channels
        spec_layer_params.hasBias = has_bias

        weights = spec_layer_params.weights
        weights.floatValue.extend(map(float, W.flatten()))
        if has_bias:
            bias = spec_layer_params.bias
            bias.floatValue.extend(map(float, b.flatten()))


    def add_softmax(self, name, input_name, output_name):
        """
        Add a softmax layer to the model.

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

        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)
        spec_layer_params = spec_layer.softmax.MergeFromString(b'')

    def add_activation(self, name, non_linearity, input_name, output_name,
        params=None):
        """
        Add an activation layer to the model.

        Parameters
        ----------
        name: str
            The name of this layer
        non_linearity: str
            The non_linearity (activation) function of this layer.
            It can be one of the following:

                - 'RELU': Rectified Linear Unit (ReLU) function.
                - 'SIGMOID': sigmoid function.
                - 'TANH': tanh function.
                - 'SCALED_TANH': scaled tanh function, defined as:

                  `f(x) = alpha * tanh(beta * x)`

                  where alpha and beta are constant scalars.

                - 'SOFTPLUS': softplus function.
                - 'SOFTSIGN': softsign function.
                - 'SIGMOID_HARD': hard sigmoid function, defined as:

                  `f(x) = min(max(alpha * x + beta, -1), 1)`

                  where alpha and beta are constant scalars.
                - 'LEAKYRELU': leaky relu function, defined as:

                  `f(x) = (x >= 0) * x + (x < 0) * alpha * x`

                  where alpha is a constant scalar.
                - 'PRELU': Parametric ReLU function, defined as:

                  `f(x) = (x >= 0) * x + (x < 0) * alpha * x`

                  where alpha is a multi-dimensional array of same size as x.
                - 'ELU': Exponential linear unit function, defined as:

                  `f(x) = (x >= 0) * x + (x < 0) * (alpha * exp(x) - 1)`

                  where alpha is a constant scalar.

                - 'PARAMETRICSOFTPLUS': Parametric softplus function, defined as:

                  `f(x) = alpha * log(1 + exp(beta * x))`

                  where alpha and beta are two multi-dimensional arrays of same size as x.
                - 'THRESHOLDEDRELU': Thresholded ReLU function, defined as:

                  `f(x) = (x >= alpha) * x`

                  where alpha is a constant scalar.
                - 'LINEAR': linear function.

                   `f(x) = alpha * x + beta`

        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        params: [float] | [numpy.array]
            Parameters for the activation, depending on non_linearity. Kindly refer to NeuralNetwork.proto for details.

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

        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)
        spec_layer_params = spec_layer.activation

        # Fill in the parameters
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
            # Weight alignment: Keras [H,W,C,F], Espresso [
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
            raise TypeError("Unknown activation type %s." %(non_linearity))

    def add_elementwise(self, name, input_names, output_name, mode, alpha = None):
        """
        Add an element-wise operation layer to the model.

        Parameters
        ----------
            The name of this layer
        name: str
        input_names: [str]
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
        spec = self.spec
        nn_spec = self.nn_spec

        spec_layer = nn_spec.layers.add()
        spec_layer.name = name

        if isinstance(input_names, list):
            for input_name in input_names:
                spec_layer.input.append(input_name)
        else:
            spec_layer.input.append(input_names)
        spec_layer.output.append(output_name)

        ## Add the following layers.
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
            raise ValueError("Unsupported elementwise mode %s" % mode)

    def add_upsample(self, name, scaling_factor_h, scaling_factor_w, input_name, output_name, mode = 'NN'):
        """
        Add upsample layer to the model.

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
            'BILINEAR' : bilinear interpolation

        See Also
        --------
        add_sequence_repeat, add_elementwise
        """
        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new inner-product layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)
        spec_layer_params = spec_layer.upsample
        spec_layer_params.scalingFactor.append(scaling_factor_h)
        spec_layer_params.scalingFactor.append(scaling_factor_w)
        if mode == 'NN':
            spec_layer_params.mode = _NeuralNetwork_pb2.UpsampleLayerParams.InterpolationMode.Value('NN')
        elif mode == 'BILINEAR':
            spec_layer_params.mode = _NeuralNetwork_pb2.UpsampleLayerParams.InterpolationMode.Value('BILINEAR')
        else:
            raise ValueError("Unsupported upsampling mode %s" % mode)

    def add_scale(self, name, W, b, has_bias, input_name, output_name, shape_scale = [1], shape_bias = [1]):
        """
        Add scale layer to the model.

        Parameters
        ----------
        name: str
            The name of this layer.
        W: int | numpy.array
            Scale of the input.
        b: int | numpy.array
            Bias to add to the input.
        has_bias: boolean
            Whether the bias vector of this layer is ignored in the spec.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        shape_scale: [int]
            List of ints that specifies the shape of the scale parameter. Can be [1] or [C] or [1,H,W] or [C,H,W].
        shape_bias: [int]
            List of ints that specifies the shape of the bias parameter (if present). Can be [1] or [C] or [1,H,W] or [C,H,W].

        See Also
        --------
        add_bias
        """
        spec = self.spec
        nn_spec = self.nn_spec

        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)
        spec_layer_params = spec_layer.scale

        spec_layer_params.hasBias = has_bias

        #add scale and its shape
        scale = spec_layer_params.scale
        spec_layer_params.shapeScale.extend(shape_scale)
        if isinstance(W, int):
            scale.floatValue.append(float(W))
        else:
            scale.floatValue.extend(map(float, W.flatten()))
        if len(scale.floatValue) != np.prod(shape_scale):
            raise ValueError("Dimensions of 'shape_scale' do not match the size of the provided 'scale' parameter")

        #add bias and its shape
        if has_bias:
            bias = spec_layer_params.bias
            spec_layer_params.shapeBias.extend(shape_bias)
            if isinstance(b, int):
                bias.floatValue.append(float(b))
            else:
                bias.floatValue.extend(map(float, b.flatten()))
            if len(bias.floatValue) != np.prod(shape_bias):
                raise ValueError("Dimensions of 'shape_bias' do not match the size of the provided 'b' parameter")

    def add_bias(self, name, b, input_name, output_name, shape_bias = [1]):
        """
        Add bias layer to the model.

        Parameters
        ----------
        name: str
            The name of this layer.
        b: int | numpy.array
            Bias to add to the input.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.
        shape_bias: [int]
            List of ints that specifies the shape of the bias parameter (if present). Can be [1] or [C] or [1,H,W] or [C,H,W].

        See Also
        --------
        add_scale
        """
        spec = self.spec
        nn_spec = self.nn_spec

        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)
        spec_layer_params = spec_layer.bias

        #add bias and its shape
        bias = spec_layer_params.bias
        spec_layer_params.shape.extend(shape_bias)
        if isinstance(b, int):
            bias.floatValue.append(float(b))
        else:
            bias.floatValue.extend(map(float, b.flatten()))
        if len(bias.floatValue) != np.prod(shape_bias):
            raise ValueError("Dimensions of 'shape_bias' do not match the size of the provided 'b' parameter")

    def add_sequence_repeat(self, name, nrep, input_name, output_name):
        """
        Add sequence repeat layer to the model.

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
        spec = self.spec
        nn_spec = self.nn_spec
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)
        spec_layer_params = spec_layer.sequenceRepeat
        spec_layer_params.nRepetitions = nrep

    def add_convolution(self, name, kernel_channels, output_channels, height,
            width, stride_height, stride_width, border_mode, groups, W, b, has_bias,
            is_deconv = False, output_shape = None,
            input_name = 'data', output_name = 'out',
            dilation_factors = [1,1],
            padding_top = 0, padding_bottom = 0, padding_left = 0, padding_right = 0,
            same_padding_asymmetry_mode = 'BOTTOM_RIGHT_HEAVY'):
        """
        Add a convolution layer to the network.

        Please see the ConvolutionLayerParams in Core ML neural network
        protobuf message for more information about input and output blob dimensions.

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
            Kindly refer to NeuralNetwork.proto for details.
        groups: int
            Number of kernel groups. Input is divided into groups along the channel axis. Each kernel group share the same weights.
        W: numpy.array
            Weights of the convolution kernels.

            - If is_deconv is False, W should have shape (height, width, kernel_channels, output_channels), where kernel_channel = input_channels / groups
            - If is_deconv is True, W should have shape (height, width, kernel_channels, output_channels / groups), where kernel_channel = input_channels

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

        output_shape: tuple | None
            Either None or a 2-tuple, specifying the output shape (output_height, output_width). Used only when is_deconv == True.
            When is_deconv == False, this parameter is ignored.
            If it is None, the output shape is calculated automatically using the border_mode.
            Kindly refer to NeuralNetwork.proto for details.

        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        dilation_factors: [int]
            Dilation factors across height and width directions. Must be a list of two positive integers.
            Defaults to [1,1]

        padding_top, padding_bottom, padding_left, padding_right: int
            values of height (top, bottom) and width (left, right) padding to be used if border_more is "valid".

        same_padding_asymmetry_mode : str.
            Type of asymmetric padding to be used when  border_mode is 'same'.
            Can be either 'BOTTOM_RIGHT_HEAVY' or  'TOP_LEFT_HEAVY'.
            Kindly refer to NeuralNetwork.proto for details.


        Depthwise convolution is a special case of convolution, where we have:
            kernel_channels = 1 (== input_channels / groups)
            output_channels = channel_multiplier * input_channels
            groups = input_channels
            W : [Kernel_height, Kernel_width, 1, channel_multiplier * input_channels]


        See Also
        --------
        add_pooling, add_activation, add_batchnorm

        """
        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)
        spec_layer.convolution.MergeFromString(b'') # hack to set empty message

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

        if border_mode == 'valid':
            height_border = spec_layer_params.valid.paddingAmounts.borderAmounts.add()
            height_border.startEdgeSize = padding_top
            height_border.endEdgeSize = padding_bottom
            width_border = spec_layer_params.valid.paddingAmounts.borderAmounts.add()
            width_border.startEdgeSize = padding_left
            width_border.endEdgeSize = padding_right
        elif border_mode == 'same':
            if not (same_padding_asymmetry_mode == 'BOTTOM_RIGHT_HEAVY' or  same_padding_asymmetry_mode == 'TOP_LEFT_HEAVY'):
                raise ValueError("Invalid value %d of same_padding_asymmetry_mode parameter" % same_padding_asymmetry_mode)
            spec_layer_params.same.asymmetryMode = _NeuralNetwork_pb2.SamePadding.SamePaddingMode.Value(same_padding_asymmetry_mode)
        else:
            raise NotImplementedError(
                'Border mode %s is not implemented.' % border_mode)

        spec_layer_params.nGroups = groups
        spec_layer_params.hasBias = has_bias

        # Assign weights
        weights = spec_layer_params.weights

        # Weight alignment: MLModel Spec requires following weight arrangement:
        # is_deconv == False ==> (output_channels, kernel_channels, height, width), where kernel_channel = input_channels / groups
        # is_deconv == True ==> (kernel_channels, output_channels / groups, height, width), where kernel_channel = input_channels
        if not is_deconv:
            Wt = W.transpose((3,2,0,1))
            Wt = Wt.flatten()
        else:
            Wt = W.transpose((2,3,0,1)).flatten()
        for idx in range(Wt.size):
            weights.floatValue.append(float(Wt[idx]))

        # Assign biases
        if has_bias:
            bias = spec_layer_params.bias
            for f in range(output_channels):
                bias.floatValue.append(float(b[f]))

        # add dilation factors
        spec_layer_params.dilationFactor.append(dilation_factors[0])
        spec_layer_params.dilationFactor.append(dilation_factors[1])

    def add_pooling(self, name, height, width, stride_height, stride_width,
            layer_type, padding_type, input_name, output_name, exclude_pad_area = True, is_global = False,
            padding_top = 0, padding_bottom = 0, padding_left = 0, padding_right = 0,
            same_padding_asymmetry_mode = 'BOTTOM_RIGHT_HEAVY'):
        """
        Add a pooling layer to the model.

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
            Kindly refer to NeuralNetwork.proto for details.
        input_name: str
            The input blob name of this layer.
        output_name: str
            The output blob name of this layer.

        exclude_pad_area: boolean
            Whether to exclude padded area in the 'AVERAGE' pooling operation. Defaults to True.

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

        same_padding_asymmetry_mode : str.
            Type of asymmetric padding to be used when  padding_type = 'SAME'.
            Can be either 'BOTTOM_RIGHT_HEAVY' or  'TOP_LEFT_HEAVY'.
            Kindly refer to NeuralNetwork.proto for details.

        See Also
        --------
        add_convolution, add_activation
        """

        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)
        spec_layer_params = spec_layer.pooling

        # Set the parameters
        spec_layer_params.type = \
                    _NeuralNetwork_pb2.PoolingLayerParams.PoolingType.Value(layer_type)

        if padding_type == 'VALID':
            height_border = spec_layer_params.valid.paddingAmounts.borderAmounts.add()
            height_border.startEdgeSize = padding_top
            height_border.endEdgeSize = padding_bottom
            width_border = spec_layer_params.valid.paddingAmounts.borderAmounts.add()
            width_border.startEdgeSize = padding_left
            width_border.endEdgeSize = padding_right
        elif padding_type == 'SAME':
            if not (same_padding_asymmetry_mode == 'BOTTOM_RIGHT_HEAVY' or  same_padding_asymmetry_mode == 'TOP_LEFT_HEAVY'):
                raise ValueError("Invalid value %d of same_padding_asymmetry_mode parameter" % same_padding_asymmetry_mode)
            spec_layer_params.same.asymmetryMode = _NeuralNetwork_pb2.SamePadding.SamePaddingMode.Value(same_padding_asymmetry_mode)
        elif padding_type == 'INCLUDE_LAST_PIXEL':
            if padding_top != padding_bottom or padding_left != padding_right:
                raise ValueError("Only symmetric padding is supported with the INCLUDE_LAST_PIXEL padding type")
            spec_layer_params.includeLastPixel.paddingAmounts.append(padding_top)
            spec_layer_params.includeLastPixel.paddingAmounts.append(padding_left)
        else:
            raise ValueError("Unknown padding_type %s in pooling" %(padding_type))


        spec_layer_params.kernelSize.append(height)
        spec_layer_params.kernelSize.append(width)
        spec_layer_params.stride.append(stride_height)
        spec_layer_params.stride.append(stride_width)
        spec_layer_params.avgPoolExcludePadding = exclude_pad_area
        spec_layer_params.globalPooling = is_global

    def add_padding(self, name,
            left = 0, right = 0, top = 0, bottom = 0,
            value = 0,
            input_name = 'data', output_name = 'out',
            padding_type = 'constant'):
        """
        Add a padding layer to the model. Kindly refer to NeuralNetwork.proto for details.

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
            Type of the padding. Can be one of 'constant', 'reflection' or 'replication'

        See Also
        --------
        add_crop, add_convolution, add_pooling
        """
        # Currently only constant padding is supported.
        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)
        spec_layer_params = spec_layer.padding

        # Set the parameters
        if padding_type == 'constant':
            spec_layer_params.constant.value = value
        elif padding_type == 'reflection':
            spec_layer_params.reflection.MergeFromString(b'')
        elif padding_type == 'replication':
            spec_layer_params.replication.MergeFromString(b'')
        else:
            raise ValueError("Unknown padding_type %s" %(padding_type))


        height_border = spec_layer_params.paddingAmounts.borderAmounts.add()
        height_border.startEdgeSize = top
        height_border.endEdgeSize = bottom
        width_border = spec_layer_params.paddingAmounts.borderAmounts.add()
        width_border.startEdgeSize = left
        width_border.endEdgeSize = right

    def add_crop(self, name, left, right, top, bottom, offset, input_names,
            output_name):
        """
        Add a cropping layer to the model.
        The cropping layer have two functional modes:

            - When it has 1 input blob, it crops the input blob based
              on the 4 parameters [left, right, top, bottom].
            - When it has 2 input blobs, it crops the first input blob based
              on the dimension of the second blob with an offset.

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
        offset: [int]
            Offset along the height and width directions when the crop layer takes 2 inputs. Must be a list of length 2.
            When the crop layer takes 1 input, this parameter is ignored.
        input_names: [str]
            The input blob name(s) of this layer. Must be either a list of 1 string (1 input crop layer),
            or a list of 2 strings (2-input crop layer).
        output_name: str
            The output blob name of this layer.

        See Also
        --------
        add_padding, add_convolution, add_pooling
        """
        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        for input_name in input_names:
            spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)
        spec_layer_params = spec_layer.crop

        # Set the parameters
        offset = [0,0] if len(input_name) == 1 else offset
        spec_layer_params.offset.extend(offset)
        height_border = spec_layer_params.cropAmounts.borderAmounts.add()
        height_border.startEdgeSize = top
        height_border.endEdgeSize = bottom
        width_border = spec_layer_params.cropAmounts.borderAmounts.add()
        width_border.startEdgeSize = left
        width_border.endEdgeSize = right

    def add_simple_rnn(self,name, W_h, W_x, b, hidden_size, input_size, activation, input_names, output_names, output_all = False, reverse_input = False):
        """
        Add a simple recurrent layer to the model.

        Parameters
        ----------
        name: str
            The name of this layer.
        W_h: numpy.array
            Weights of the recurrent layer's hidden state. Must be of shape (hidden_size, hidden_size).
        W_x: numpy.array
            Weights of the recurrent layer's input. Must be of shape (hidden_size, input_size).
        b: numpy.array | None
            Bias of the recurrent layer's output. If None, bias is ignored. Otherwise it must be of shape (hidden_size, ).
        hidden_size: int
            Number of hidden units. This is equal to the number of channels of output shape.
        input_size: int
            Number of the number of channels of input shape.
        activation: str
            Activation function name. Can be one of the following option:
            ['RELU', 'TANH', 'SIGMOID', 'SCALED_TANH', 'SIGMOID_HARD', 'LINEAR'].
            See add_activation for more detailed description.
        input_names: [str]
            The input blob name list of this layer, in the order of [x, h_input].
        output_name: [str]
            The output blob name list of this layer, in the order of [y, h_output].
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

        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new Layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        for name in input_names:
            spec_layer.input.append(name)
        for name in output_names:
            spec_layer.output.append(name)
        spec_layer_params = spec_layer.simpleRecurrent
        spec_layer_params.reverseInput = reverse_input

        #set the parameters
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

    def add_gru(self, name, W_h, W_x, b, hidden_size, input_size,
            input_names, output_names, activation = 'TANH', inner_activation = 'SIGMOID_HARD',
            output_all = False, reverse_input = False):
        """
        Add a Gated-Recurrent Unit (GRU) layer to the model.

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
        b: [numpy.array] | None
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
        input_names: [str]
            The input blob name list of this layer, in the order of [x, h_input].
        output_names: [str]
            The output blob name list of this layer, in the order of [y, h_output].
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
        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new Layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name

        for name in input_names:
            spec_layer.input.append(name)
        for name in output_names:
            spec_layer.output.append(name)
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

    def add_unilstm(self, name, W_h, W_x, b, hidden_size, input_size, input_names, output_names,
                    inner_activation = 'SIGMOID',
                    cell_state_update_activation = 'TANH',
                    output_activation = 'TANH',
                    peep = None,
                    output_all = False,
                    forget_bias = False, coupled_input_forget_gate = False,
                    cell_clip_threshold = 50000.0, reverse_input = False):
        """
        Add a Uni-directional LSTM layer to the model.

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
        b: [numpy.array] | None
            List of biases. The ordering is [b_i, b_f, b_o, b_z],
            where b_i, b_f, b_o, b_z are biases at input gate, forget gate, output gate and cell gate.
            If None, biases are ignored. Otherwise the shapes of the biases are (hidden_size, ).
        hidden_size: int
            Number of hidden units. This is equal to the number of channels of output shape.
        input_size: int
            Number of the number of channels of input shape.
        input_names: [str]
            The input blob name list of this layer, in the order of [x, h_input, c_input].
        output_names: [str]
            The output blob name list of this layer, in the order of [y, h_output, c_output].
        inner_activation: str
            Inner activation function used at input and forget gate. Can be one of the following option:
            ['RELU', 'TANH', 'SIGMOID', 'SCALED_TANH', 'SIGMOID_HARD', 'LINEAR'].
        cell_state_update_activation: str
            Cell state update activation function used at the cell state update gate.
            ['RELU', 'TANH', 'SIGMOID', 'SCALED_TANH', 'SIGMOID_HARD', 'LINEAR'].
        output_activation: str
            Activation function used at the output gate. Can be one of the following option:
            ['RELU', 'TANH', 'SIGMOID', 'SCALED_TANH', 'SIGMOID_HARD', 'LINEAR'].
        peep: [numpy.array] | None
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
        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new Layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        for name in input_names:
            spec_layer.input.append(name)
        for name in output_names:
            spec_layer.output.append(name)
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

    def add_bidirlstm(self, name, W_h, W_x, b, W_h_back, W_x_back, b_back, hidden_size, input_size,
            input_names, output_names,
            inner_activation = 'SIGMOID',
            cell_state_update_activation = 'TANH',
            output_activation = 'TANH',
            peep = None, peep_back = None,
            output_all = False,
            forget_bias = False, coupled_input_forget_gate= False, cell_clip_threshold = 50000.0):

        """
        Add a Bi-directional LSTM layer to the model.

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
        input_names: [str]
            The input blob name list of this layer, in the order of [x, h_input, c_input, h_reverse_input, c_reverse_input].
        output_names: [str]
            The output blob name list of this layer, in the order of [y, h_output, c_output, h_reverse_output, c_reverse_output].
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
        peep: [numpy.array] | None
            List of peephole vectors for the forward layer. The ordering is [p_i, p_f, p_o],
            where p_i, p_f, and p_o are peephole vectors at input gate, forget gate, output gate.
            The shapes of the peephole vectors are (hidden_size,). Defaults to None.
        peep_back: [numpy.array] | None
            List of peephole vectors for the backward layer. The ordering is [p_i, p_f, p_o],
            where p_i, p_f, and p_o are peephole vectors at input gate, forget gate, output gate.
            The shapes of the peephole vectors are (hidden_size,). Defaults to None.
        output_all: boolean
            Whether the LSTM layer should output at every time step. Defaults to False.

            - If False, the output is the result after the final state update.
            - If True, the output is a sequence, containing outputs at all time steps.

        forget_bias: boolean
            If True, a vector of 1s is added to forget gate bias. Defaults to False.
        coupled_input_forget_gate : boolean
            If True, the input gate and forget gate is coupled. i.e. forget gate is not used.
            Defaults to False.
        cell_clip_threshold : float
            The limit on the maximum and minimum values on the cell state.
            Defaults to 50.0.

        See Also
        --------
        add_activation, add_simple_rnn, add_unilstm, add_bidirlstm
        """

        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new Layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        for name in input_names:
            spec_layer.input.append(name)
        for name in output_names:
            spec_layer.output.append(name)
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

        #set activations
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

    def add_flatten(self, name, mode, input_name, output_name):
        """
        Add a flatten layer. Only flattens the channel, height and width axis. Leaves the sequence axis as is.

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
        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)
        spec_layer_params = spec_layer.flatten

        # Set the parameters
        if mode == 0:
            spec_layer_params.mode = \
                        _NeuralNetwork_pb2.FlattenLayerParams.FlattenOrder.Value('CHANNEL_FIRST')
        elif mode == 1:
            spec_layer_params.mode = \
                        _NeuralNetwork_pb2.FlattenLayerParams.FlattenOrder.Value('CHANNEL_LAST')
        else:
            raise NotImplementedError(
                'Unknown flatten mode %d ' % mode)

    def add_slice(self, name, input_name, output_name, axis, start_index = 0, end_index = -1, stride = 1):
        """
        Add a slice layer. Equivalent to to numpy slice [start_index:end_index:stride],
        start_index is included, while end_index is exclusive.

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
        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)
        spec_layer_params = spec_layer.slice

        # Set the parameters
        if start_index < 0:
            raise ValueError("Invalid start_index value %d. Must be non-negative." % start_index)
        if stride < 1:
            raise ValueError("Invalid stride value %d. Must be positive." % stride)

        spec_layer_params.startIndex = start_index
        spec_layer_params.endIndex = end_index
        spec_layer_params.stride = stride

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
            raise NotImplementedError(
                'Unsupported Slice axis %s ' % axis)


    def add_reorganize_data(self, name, input_name, output_name, mode = 'SPACE_TO_DEPTH', block_size = 2):
        """
        Add a data reorganization layer of type "SPACE_TO_DEPTH" or "DEPTH_TO_SPACE".

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
        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)
        spec_layer_params = spec_layer.reorganizeData

        # Set the parameters
        if block_size < 2:
            raise ValueError("Invalid block_size value %d. Must be greater than 1." % block_size)
        spec_layer_params.blockSize = block_size
        if mode == 'SPACE_TO_DEPTH':
            spec_layer_params.mode = \
                        _NeuralNetwork_pb2.ReorganizeDataLayerParams.ReorganizationType.Value('SPACE_TO_DEPTH')
        elif mode == 'DEPTH_TO_SPACE':
            spec_layer_params.mode = \
                        _NeuralNetwork_pb2.ReorganizeDataLayerParams.ReorganizationType.Value('DEPTH_TO_SPACE')
        else:
            raise NotImplementedError(
                'Unknown reorganization mode %s ' % mode)

    def add_batchnorm(self, name, channels, gamma, beta,
                      mean = None, variance = None,
                      input_name = 'data', output_name = 'out',
                      compute_mean_var = False,
                      instance_normalization = False, epsilon = 1e-5):
        """
        Add a Batch Normalization layer. Batch Normalization operation is
        defined as:

        `y = gamma * (x - mean) / sqrt(variance + epsilon) + beta`
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

        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)

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


    def add_permute(self, name, dim, input_name, output_name):
        """
        Add a permute layer. Assumes that the input has dimensions in the order [Seq, C, H, W]

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
        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)

        spec_layer_params = spec_layer.permute
        spec_layer_params.axis.extend(list(dim))

        if len(dim) != 4:
            raise ValueError("Length of the 'dim' parameter must be equal to 4")

    def add_reshape(self, name, input_name, output_name, target_shape, mode):
        """
        Add a reshape layer. Kindly refer to NeuralNetwork.proto for details.

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

        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)

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

    def add_reduce(self, name, input_name, output_name, axis, mode, epsilon = 1e-6):
        """
        Add a reduce layer. Applies the function specified by the parameter mode,
        along dimension(s) specified by the parameter axis.

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
            'argmax' is only suuported with axis values 'C', 'H' and 'W'.

        epsilon: float
            number that is added to the input when 'logsum' function is applied.

        See Also
        --------
        add_activation
        """

        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)

        spec_layer_params = spec_layer.reduce
        spec_layer_params.epsilon = epsilon

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
        elif mode == 'L1':
            spec_layer_params.mode = _NeuralNetwork_pb2.ReduceLayerParams.ReduceOperation.Value('L1')
        elif mode == 'L2':
            spec_layer_params.mode = _NeuralNetwork_pb2.ReduceLayerParams.ReduceOperation.Value('L2')
        elif mode == 'max':
            spec_layer_params.mode = _NeuralNetwork_pb2.ReduceLayerParams.ReduceOperation.Value('MAX')
        elif mode == 'min':
            spec_layer_params.mode = _NeuralNetwork_pb2.ReduceLayerParams.ReduceOperation.Value('MIN')
        elif mode == 'argmax':
            spec_layer_params.mode = _NeuralNetwork_pb2.ReduceLayerParams.ReduceOperation.Value('ARGMAX')
        else:
            raise NotImplementedError('Unknown reduction operation %s ' % mode)

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
            raise NotImplementedError('Unknown reduction axis %s ' % axis)


    def add_lrn(self, name, input_name, output_name, alpha, beta, local_size, k = 1.0):
        """
        Add a LRN (local response normalization) layer. Please see the LRNLayerParams message in Core ML neural network
        protobuf for more information about the operation of this layer. Supports "across" channels normalization.

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

        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)

        spec_layer_params = spec_layer.lrn
        spec_layer_params.alpha = alpha
        spec_layer_params.beta = beta
        spec_layer_params.localSize = local_size
        spec_layer_params.k = k

    def add_mvn(self, name, input_name, output_name, across_channels = True, normalize_variance = True, epsilon = 1e-5):
        """
        Add an MVN (mean variance normalization) layer. Computes mean, variance and normalizes the input.

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

        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)

        spec_layer_params = spec_layer.mvn
        spec_layer_params.acrossChannels = across_channels
        spec_layer_params.normalizeVariance = normalize_variance
        spec_layer_params.epsilon = epsilon


    def add_l2_normalize(self, name, input_name, output_name, epsilon = 1e-5):
        """
        Add L2 normalize layer. Normalizes the input by the L2 norm, i.e. divides by the
        the square root of the sum of squares of all elements of the input along C, H and W dimensions.

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

        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)

        spec_layer_params = spec_layer.l2normalize
        spec_layer_params.epsilon = epsilon


    def add_unary(self, name, input_name, output_name, mode, alpha = 1.0,
                    shift = 0, scale = 1.0, epsilon = 1e-6):
        """
        Add a Unary layer. Applies the specified function (mode) to all the elements of the input.
        Please see the UnaryFunctionLayerParams message in Core ML neural network
        protobuf for more information about the operation of this layer.
        Prior to the application of the function the input can be scaled and shifted by using the 'scale',
        'shift' parameters.

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

        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.append(output_name)

        spec_layer_params = spec_layer.unary
        spec_layer_params.epsilon = epsilon
        spec_layer_params.alpha = alpha
        spec_layer_params.shift = shift
        spec_layer_params.scale = scale

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

    def add_split(self, name, input_name, output_names):
        """
        Add a Split layer that uniformly splits the input along the channel dimension
        to produce multiple outputs.

        Parameters
        ----------
        name: str
            The name of this layer.

        input_name: str
            The input blob name of this layer.
        output_names: [str]
            List of output blob names of this layer.

        See Also
        --------
        add_elementwise
        """

        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.input.append(input_name)
        spec_layer.output.extend(output_names)

        spec_layer_params = spec_layer.split
        spec_layer_params.nOutputs = len(output_names)

    def add_load_constant(self, name, output_name, constant_value, shape):
        """
        Add a load constant layer.

        Parameters
        ----------
        name: str
            The name of this layer.

        output_name: str
            The output blob name of this layer.

        constant_value: numpy.array
            value of the constant as a numpy array.

        shape: [int]
            List of ints representing the shape of the constant. Must be of length 3: [C,H,W]


        See Also
        --------
        add_elementwise
        """
        spec = self.spec
        nn_spec = self.nn_spec

        # Add a new layer
        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        spec_layer.output.append(output_name)

        spec_layer_params = spec_layer.loadConstant

        data = spec_layer_params.data
        data.floatValue.extend(map(float, constant_value.flatten()))

        spec_layer_params.shape.extend(shape)

        if len(data.floatValue) != np.prod(shape):
            raise ValueError("Dimensions of 'shape' do not match the size of the provided constant")
        if len(shape) != 3:
            raise ValueError("'shape' must be of length 3")


    def add_custom(self, name, input_names, output_names, custom_proto_spec = None):
        """
        Add a custom layer.

        Parameters
        ----------
        name: str
            The name of this layer.

        input_names: [str]
            The input blob names to this layer.

        output_names: [str]
            The output blob names from this layer.

        custom_proto_spec: CustomLayerParams
            A protobuf CustomLayerParams message. This can also be left blank and filled in later.
        """

        spec = self.spec
        nn_spec = self.nn_spec

        # custom layers require a newer specification version
        from coremltools import _MINIMUM_CUSTOM_LAYER_SPEC_VERSION
        spec.specificationVersion = max(spec.specificationVersion, _MINIMUM_CUSTOM_LAYER_SPEC_VERSION)

        spec_layer = nn_spec.layers.add()
        spec_layer.name = name
        for inname in input_names:
            spec_layer.input.append(inname)
        for outname in output_names:
            spec_layer.output.append(outname)

        # Have to do it this way since I can't just assign custom in a layer
        spec_layer.custom.MergeFromString(b'')
        if custom_proto_spec:
            spec_layer.custom.CopyFrom(custom_proto_spec)


    def set_pre_processing_parameters(self, image_input_names = [], is_bgr = False,
            red_bias = 0.0, green_bias = 0.0, blue_bias = 0.0, gray_bias = 0.0, image_scale = 1.0):
        """Add pre-processing parameters to the neural network object

        Parameters
        ----------
        image_input_names: [str]
            Name of input blobs that are images

        is_bgr: boolean | dict()
            Channel order for input blobs that are images. BGR if True else RGB.
            To specify a different value for each image input,
            provide a dictionary with input names as keys.

        red_bias: float | dict()
            Image re-centering parameter (red channel)

        blue_bias: float | dict()
            Image re-centering parameter (blue channel)

        green_bias: float | dict()
            Image re-centering parameter (green channel)

        gray_bias: float | dict()
            Image re-centering parameter (for grayscale images)

        image_scale: float | dict()
            Value by which to scale the images.

        See Also
        --------
        set_input, set_output, set_class_labels
        """
        spec = self.spec
        if not image_input_names:
            return # nothing to do here


        if not isinstance(is_bgr, dict): is_bgr = dict.fromkeys(image_input_names, is_bgr)
        if not isinstance(red_bias, dict): red_bias = dict.fromkeys(image_input_names, red_bias)
        if not isinstance(blue_bias, dict): blue_bias = dict.fromkeys(image_input_names, blue_bias)
        if not isinstance(green_bias, dict): green_bias = dict.fromkeys(image_input_names, green_bias)
        if not isinstance(gray_bias, dict): gray_bias = dict.fromkeys(image_input_names, gray_bias)
        if not isinstance(image_scale, dict): image_scale = dict.fromkeys(image_input_names, image_scale)

        # Add image inputs
        for input_ in spec.description.input:
            if input_.name in image_input_names:
                if input_.type.WhichOneof('Type') == 'multiArrayType':
                    array_shape = tuple(input_.type.multiArrayType.shape)
                    channels, height, width = array_shape
                    if channels == 1:
                        input_.type.imageType.colorSpace = _FeatureTypes_pb2.ImageFeatureType.ColorSpace.Value('GRAYSCALE')
                    elif channels == 3:
                        if input_.name in is_bgr:
                            if is_bgr[input_.name]:
                                input_.type.imageType.colorSpace = _FeatureTypes_pb2.ImageFeatureType.ColorSpace.Value('BGR')
                            else:
                                input_.type.imageType.colorSpace = _FeatureTypes_pb2.ImageFeatureType.ColorSpace.Value('RGB')
                        else:
                            input_.type.imageType.colorSpace = _FeatureTypes_pb2.ImageFeatureType.ColorSpace.Value('RGB')
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
