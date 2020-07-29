# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

"""
Functions related to graph visualization of mlmodels
"""

import ast as _ast
import json as _json
import os as _os
import numpy as _np
from ._infer_shapes_nn_mlmodel import infer_shapes as _infer_shapes
from coremltools.proto import NeuralNetwork_pb2 as _NeuralNetwork_pb2


def _calculate_edges(cy_nodes, cy_edges, shape_dict=None):
    """

    Parameters
    ----------
    cy_nodes: list of nodes for graph
    cy_edges: list of edges to be updated for graph
    shape_dict: shape_dict required for inferring shape information

    Returns
    -------

    cy_nodes: list of nodes for graph
    cy_edges: list of edges to be updated for graph

    """
    node_len = len(cy_nodes)

    for upper_index in range(0, node_len):
        for lower_index in range(upper_index + 1, node_len):

            if (
                "outputs" in cy_nodes[upper_index]["data"]["info"].keys()
                and "inputs" in cy_nodes[upper_index]["data"]["info"].keys()
                and "outputs" in cy_nodes[lower_index]["data"]["info"].keys()
                and "inputs" in cy_nodes[lower_index]["data"]["info"].keys()
            ):
                outputs = _ast.literal_eval(
                    cy_nodes[upper_index]["data"]["info"]["outputs"]
                )
                inputs = _ast.literal_eval(
                    cy_nodes[lower_index]["data"]["info"]["inputs"]
                )
                for output in outputs:
                    if output in inputs:
                        if shape_dict is None or output not in shape_dict.keys():
                            label = None
                        else:
                            label = str(shape_dict[output])

                        cy_edges.append(
                            {
                                "data": {
                                    "id": "{}.{}.{}".format(
                                        output,
                                        cy_nodes[upper_index]["data"]["id"],
                                        cy_nodes[lower_index]["data"]["id"],
                                    ),
                                    "source": cy_nodes[upper_index]["data"]["id"],
                                    "target": cy_nodes[lower_index]["data"]["id"],
                                    "label": label,
                                    "shape": label,
                                }
                            }
                        )

    return cy_nodes, cy_edges


def _layer_specific_info(layer):
    """

    Parameters
    ----------
    layer : Can be one of : 'activation', 'add', 'average', 'batchnorm',
    'biDirectionalLSTM', 'bias', 'concat', 'convolution', 'crop', 'dot',
    'embedding', 'flatten', 'gru', 'innerProduct', 'input', 'l2normalize',
    'loadConstant', 'lrn', 'max', 'min', 'multiply', 'mvn', 'name', 'output',
    'padding', permute', 'pooling', 'reduce', 'reorganizeData', 'reshape',
    'scale', 'sequenceRepeat', 'simpleRecurrent', 'slice', 'softmax', 'split',
     'unary', 'uniDirectionalLSTM', 'upsample'

    Returns
    -------
    info : info specific to layer type

    """
    if layer.WhichOneof("layer") == "convolution":
        info = {
            "type": layer.WhichOneof("layer"),
            "outputChannels": _json.dumps(str(layer.convolution.outputChannels)),
            "kernelChannels": _json.dumps(str(layer.convolution.kernelChannels)),
            "groups": _json.dumps(str(layer.convolution.nGroups)),
            "kernelSize": _json.dumps(str(layer.convolution.kernelSize)),
            "stride": _json.dumps(str(layer.convolution.stride)),
            "dilationFactor": _json.dumps(str(layer.convolution.dilationFactor)),
            "isDeconvolution": _json.dumps(str(layer.convolution.isDeconvolution)),
            "paddingType": _json.dumps(
                layer.convolution.WhichOneof("ConvolutionPaddingType")
            ),
            "desc": "A layer that performs spatial convolution",
        }
        if _json.dumps(layer.convolution.isDeconvolution) == "true":
            info["type"] = "deconvolution"
            info["desc"] = "A layer that performs spatial deconvolution"

    elif layer.WhichOneof("layer") == "activation":
        params = layer.activation
        act_type = params.WhichOneof("NonlinearityType")
        info = {
            "type": layer.WhichOneof("layer"),
            "activationType": act_type,
            "desc": "Applies specified type of activation function to input.",
        }
        if act_type == "linear":
            info["alpha"] = _json.dumps(str(params.linear.alpha))
            info["beta"] = _json.dumps(str(params.linear.beta))
        if act_type == "leakyReLU":
            info["alpha"] = _json.dumps(str(params.leakyReLU.alpha))
        if act_type == "thresholdedReLU":
            info["alpha"] = _json.dumps(str(params.thresholdedReLU.alpha))
        if act_type == "scaledTanh":
            info["alpha"] = _json.dumps(str(params.scaledTanh.alpha))
            info["beta"] = _json.dumps(str(params.scaledTanh.beta))
        if act_type == "sigmoidHard":
            info["alpha"] = _json.dumps(str(params.sigmoidHard.alpha))
            info["beta"] = _json.dumps(str(params.sigmoidHard.beta))
        if act_type == "ELU":
            info["alpha"] = _json.dumps(str(params.ELU.alpha))

    elif layer.WhichOneof("layer") == "pooling":
        params = layer.pooling
        paddingType = params.WhichOneof("PoolingPaddingType")
        info = {
            "type": layer.WhichOneof("layer"),
            "desc": "Spatial Pooling layer to reduce dimensions of input using the "
            "specified kernel size and type.",
        }
        if params.globalPooling:
            info["globalPooling"] = "True"
            info["poolingType"] = "global pooling"
        else:
            info["poolingType"] = _json.dumps(
                _NeuralNetwork_pb2.PoolingLayerParams.PoolingType.Name(params.type)
            )
            info["stride"] = _json.dumps(str(params.stride))
            info["kernelSize"] = _json.dumps(str(params.kernelSize))
            info["paddingType"] = _json.dumps(paddingType)

    elif layer.WhichOneof("layer") == "add":
        info = {
            "type": layer.WhichOneof("layer"),
            "alpha": _json.dumps(str(layer.add.alpha)),
            "desc": "A layer that performs elementwise addition.",
        }
    elif layer.WhichOneof("layer") == "batchnorm":
        info = {
            "type": layer.WhichOneof("layer"),
            "channels": _json.dumps(str(layer.batchnorm.channels)),
            "computeMeanVar": _json.dumps(str(layer.batchnorm.computeMeanVar)),
            "instanceNormalization": _json.dumps(
                str(layer.batchnorm.instanceNormalization)
            ),
            "desc": "A layer that performs batch normalization, \n"
            "which is performed along the channel axis, \n"
            "and repeated along the other axes, if present.",
        }
    elif layer.WhichOneof("layer") == "biDirectionalLSTM":
        forward_activations = ""
        for activation in layer.biDirectionalLSTM.activationsForwardLSTM:
            forward_activations += str(activation)[:-5] + ", "
        backward_activations = ""
        for activation in layer.biDirectionalLSTM.activationsBackwardLSTM:
            backward_activations += str(activation)[:-5] + ", "
        info = {
            "type": layer.WhichOneof("layer"),
            "inputVectorSize": _json.dumps(
                str(layer.biDirectionalLSTM.inputVectorSize)
            ),
            "outputVectorSize": _json.dumps(
                str(layer.biDirectionalLSTM.outputVectorSize)
            ),
            "forward_activations": _json.dumps(forward_activations),
            "backward_activations": _json.dumps(backward_activations),
            "lstm_params": _json.dumps(str(layer.biDirectionalLSTM.params)),
            "desc": "Bidirectional long short-term memory (LSTM) layer\n"
            "The first LSTM operates on the input sequence in the forward direction.\n"
            "The second LSTM operates on the input sequence in the reverse direction.",
        }
    elif layer.WhichOneof("layer") == "uniDirectionalLSTM":
        activations = ""
        for activation in layer.uniDirectionalLSTM.activations:
            activations += str(activation)[:-5] + ", "
        info = {
            "type": layer.WhichOneof("layer"),
            "inputVectorSize": _json.dumps(
                str(layer.uniDirectionalLSTM.inputVectorSize)
            ),
            "outputVectorSize": _json.dumps(
                str(layer.uniDirectionalLSTM.outputVectorSize)
            ),
            "activations": _json.dumps(activations),
            "lstm_params": _json.dumps(str(layer.uniDirectionalLSTM.params)),
            "reverse_input": _json.dumps(str(layer.uniDirectionalLSTM.reverseInput)),
            "desc": "A unidirectional long short-term memory (LSTM) layer.",
        }
    elif layer.WhichOneof("layer") == "gru":
        activations = ""
        for activation in layer.gru.activations:
            activations += str(activation)[:-5] + ", "
        info = {
            "type": layer.WhichOneof("layer"),
            "inputVectorSize": _json.dumps(str(layer.gru.inputVectorSize)),
            "outputVectorSize": _json.dumps(str(layer.gru.outputVectorSize)),
            "activations": _json.dumps(activations),
            "hasBiasVectors": _json.dumps(str(layer.gru.hasBiasVectors)),
            "reverseInput": _json.dumps(str(layer.gru.reverseInput)),
            "sequenceOutput": _json.dumps(str(layer.gru.sequenceOutput)),
            "desc": "Gated-Recurrent Unit (GRU) Layer.\n",
        }
    elif layer.WhichOneof("layer") == "simpleRecurrent":
        info = {
            "type": layer.WhichOneof("layer"),
            "inputVectorSize": _json.dumps(str(layer.simpleRecurrent.inputVectorSize)),
            "outputVectorSize": _json.dumps(
                str(layer.simpleRecurrent.outputVectorSize)
            ),
            "activation": _json.dumps(str(layer.simpleRecurrent.activation)),
            "hasBiasVector": _json.dumps(str(layer.simpleRecurrent.hasBiasVector)),
            "reverseInput": _json.dumps(str(layer.simpleRecurrent.reverseInput)),
            "sequenceOutput": _json.dumps(str(layer.simpleRecurrent.sequenceOutput)),
            "desc": "A simple recurrent layer.",
        }
    elif layer.WhichOneof("layer") == "bias":
        info = {
            "type": layer.WhichOneof("layer"),
            "shape": _json.dumps(str(layer.bias.shape)),
            "desc": "A layer that performs elementwise addition of a bias,\n"
            "which is broadcasted to match the input shape.",
        }
    elif layer.WhichOneof("layer") == "concat":
        info = {
            "type": layer.WhichOneof("layer"),
            "sequenceConcat": _json.dumps(str(layer.concat.sequenceConcat)),
            "desc": "A layer that concatenates along the channel axis (default) or sequence axis.",
        }
    elif layer.WhichOneof("layer") == "crop":
        info = {
            "type": layer.WhichOneof("layer"),
            "cropAmounts": _json.dumps(str(layer.crop.cropAmounts)),
            "offset": _json.dumps(str(layer.crop.offset)),
            "desc": "A layer that crops the spatial dimensions of an input.\n"
            "If two inputs are provided, the shape of the second "
            "input is used as the reference shape.",
        }
    elif layer.WhichOneof("layer") == "dot":
        info = {
            "type": layer.WhichOneof("layer"),
            "cosineSimilarity": _json.dumps(str(layer.dot.cosineSimilarity)),
            "desc": "If true, inputs are normalized first, "
            "thereby computing the cosine similarity.",
        }
    elif layer.WhichOneof("layer") == "embedding":
        info = {
            "type": layer.WhichOneof("layer"),
            "inputDim": _json.dumps(str(layer.embedding.inputDim)),
            "outputChannels": _json.dumps(str(layer.embedding.outputChannels)),
            "hasBias": _json.dumps(str(layer.embedding.inputDim)),
            "desc": "A layer that performs a matrix lookup and optionally adds a bias.",
        }
    elif layer.WhichOneof("layer") == "flatten":
        info = {
            "type": layer.WhichOneof("layer"),
            "mode": _json.dumps(
                _NeuralNetwork_pb2.FlattenLayerParams.FlattenOrder.Name(
                    layer.flatten.mode
                )
            ),
            "desc": "A layer that flattens the input.",
        }
    elif layer.WhichOneof("layer") == "innerProduct":
        info = {
            "type": layer.WhichOneof("layer"),
            "inputChannels": _json.dumps(str(layer.innerProduct.inputChannels)),
            "outputChannels": _json.dumps(str(layer.innerProduct.outputChannels)),
            "hasBias": _json.dumps(str(layer.innerProduct.hasBias)),
            "desc": "A layer that performs a matrix vector product.\n"
            "This is equivalent to a fully-connected, or dense layer.",
        }
    elif layer.WhichOneof("layer") == "l2normalize":
        info = {
            "type": layer.WhichOneof("layer"),
            "epsilon": _json.dumps(str(layer.l2normalize.epsilon)),
            "desc": "A layer that performs L2 normalization, i.e. divides by the \n"
            "the square root of the sum of squares of all elements of input.",
        }
    elif layer.WhichOneof("layer") == "loadConstant":
        info = {
            "type": layer.WhichOneof("layer"),
            "shape": _json.dumps(str(layer.loadConstant.shape)),
            "desc": "The shape of the constant to be loaded",
        }
    elif layer.WhichOneof("layer") == "lrn":
        info = {
            "type": layer.WhichOneof("layer"),
            "alpha": _json.dumps(str(layer.lrn.alpha)),
            "beta": _json.dumps(str(layer.lrn.beta)),
            "localSize": _json.dumps(str(layer.lrn.localSize)),
            "k": _json.dumps(str(layer.lrn.k)),
            "desc": "A layer that performs local response normalization (LRN).",
        }
    elif layer.WhichOneof("layer") == "multiply":
        info = {
            "type": layer.WhichOneof("layer"),
            "alpha": _json.dumps(str(layer.multiply.alpha)),
            "desc": "A layer that performs elementwise multiplication.",
        }
    elif layer.WhichOneof("layer") == "mvn":
        info = {
            "type": layer.WhichOneof("layer"),
            "acrossChannels": _json.dumps(str(layer.mvn.acrossChannels)),
            "normalizeVariance": _json.dumps(str(layer.mvn.normalizeVariance)),
            "epsilon": _json.dumps(str(layer.mvn.epsilon)),
            "desc": "A layer that performs mean variance normalization.",
        }
    elif layer.WhichOneof("layer") == "padding":
        info = {
            "type": layer.WhichOneof("layer"),
            "paddingAmounts": _json.dumps(str(layer.padding.paddingAmounts)),
            "paddingType": _json.dumps(str(layer.padding.WhichOneof("PaddingType"))),
            "desc": "Fill a constant value in the padded region.",
        }
    elif layer.WhichOneof("layer") == "permute":
        info = {
            "type": layer.WhichOneof("layer"),
            "axis": _json.dumps(str(layer.permute.axis)),
            "desc": "A layer that rearranges the dimensions and data of an input.",
        }
    elif layer.WhichOneof("layer") == "reduce":
        params = layer.reduce
        info = {
            "type": layer.WhichOneof("layer"),
            "mode": _json.dumps(str(params.mode)),
            "epsilon": _json.dumps(str(params.epsilon)),
            "axis": _json.dumps(
                _NeuralNetwork_pb2.ReduceLayerParams.ReduceAxis.Name(params.axis)
            ),
            "desc": "A layer that reduces the input using a specified operation.",
        }
    elif layer.WhichOneof("layer") == "reorganizeData":
        info = {
            "type": layer.WhichOneof("layer"),
            "mode": _json.dumps(
                _NeuralNetwork_pb2.ReorganizeDataLayerParams.ReorganizationType.Name(
                    layer.reorganizeData.mode
                )
            ),
            "blockSize": _json.dumps(str(layer.reorganizeData.blockSize)),
            "desc": "A layer that reorganizes data in the input in: \n"
            "1. SPACE_TO_DEPTH\n"
            "2. DEPTH_TO_SPACE",
        }
    elif layer.WhichOneof("layer") == "reshape":
        info = {
            "type": layer.WhichOneof("layer"),
            "mode": _json.dumps(
                _NeuralNetwork_pb2.ReshapeLayerParams.ReshapeOrder.Name(
                    layer.reshape.mode
                )
            ),
            "targetShape": _json.dumps(str(layer.reshape.targetShape)),
            "desc": "A layer that recasts the input into a new shape.",
        }
    elif layer.WhichOneof("layer") == "scale":
        info = {
            "type": layer.WhichOneof("layer"),
            "shapeScale": _json.dumps(str(layer.scale.shapeScale)),
            "hasBias": _json.dumps(str(layer.scale.hasBias)),
            "shapeBias": _json.dumps(str(layer.scale.shapeBias)),
            "desc": "A layer that performs elmentwise multiplication by a scale factor\n"
            "and optionally adds a bias;",
        }
    elif layer.WhichOneof("layer") == "sequenceRepeat":
        info = {
            "type": layer.WhichOneof("layer"),
            "nRepetitions": _json.dumps(str(layer.sequenceRepeat.nRepetitions)),
            "desc": "A layer that repeats a sequence.",
        }
    elif layer.WhichOneof("layer") == "slice":
        info = {
            "type": layer.WhichOneof("layer"),
            "startIndex": _json.dumps(str(layer.slice.startIndex)),
            "endIndex": _json.dumps(str(layer.slice.endIndex)),
            "stride": _json.dumps(str(layer.slice.stride)),
            "axis": _json.dumps(
                _NeuralNetwork_pb2.SliceLayerParams.SliceAxis.Name(layer.slice.axis)
            ),
            "desc": "A layer that slices the input data along a given axis.",
        }
    elif layer.WhichOneof("layer") == "split":
        info = {
            "type": layer.WhichOneof("layer"),
            "nOutputs": _json.dumps(str(layer.split.nOutputs)),
            "desc": "A layer that uniformly splits across the channel dimension\n"
            "to produce a specified number of outputs.",
        }
    elif layer.WhichOneof("layer") == "unary":
        info = {
            "type": layer.WhichOneof("layer"),
            "unary_type": _json.dumps(
                _NeuralNetwork_pb2.UnaryFunctionLayerParams.Operation.Name(
                    layer.unary.type
                )
            ),
            "alpha": _json.dumps(str(layer.unary.alpha)),
            "epsilon": _json.dumps(str(layer.unary.epsilon)),
            "shift": _json.dumps(str(layer.unary.shift)),
            "scale": _json.dumps(str(layer.unary.scale)),
            "desc": "A layer that applies a unary function.",
        }
    elif layer.WhichOneof("layer") == "upsample":
        info = {
            "type": layer.WhichOneof("layer"),
            "scalingFactor": _json.dumps(str(layer.upsample.scalingFactor)),
            "mode": _json.dumps(
                _NeuralNetwork_pb2.UpsampleLayerParams.InterpolationMode.Name(
                    layer.upsample.mode
                )
            ),
            "desc": "A layer that scales up spatial dimensions.\n"
            "It supports two modes: "
            "nearest neighbour (default) and bilinear.",
        }
    elif layer.WhichOneof("layer") == "max":
        info = {
            "type": layer.WhichOneof("layer"),
            "desc": "A layer that computes the elementwise maximum " "over the inputs.",
        }
    elif layer.WhichOneof("layer") == "min":
        info = {
            "type": layer.WhichOneof("layer"),
            "desc": "A layer that computes the elementwise minimum " "over the inputs.",
        }
    elif layer.WhichOneof("layer") == "average":
        info = {
            "type": layer.WhichOneof("layer"),
            "desc": "A layer that computes the elementwise average " "of the inputs.",
        }
    elif layer.WhichOneof("layer") == "softmax":
        info = {
            "type": layer.WhichOneof("layer"),
            "desc": "A layer that performs softmax normalization.\n"
            "Normalization is done along the channel axis.",
        }
    elif layer.WhichOneof("layer") == "custom":
        info = {
            "type": layer.WhichOneof("layer"),
            "className": layer.custom.className,
            "desc": "A custom layer",
        }
        if layer.custom.parameters != {}:
            for key in layer.custom.parameters.keys():
                value = _get_custom_layer_value(layer.custom.parameters[key])
                info[key] = value
        if layer.custom.description:
            info["desc"] = layer.custom.description

    else:
        info = {"type": layer.WhichOneof("layer")}

    info["inputs"] = str(layer.input)
    info["outputs"] = str(layer.output)

    return info


def _get_custom_layer_value(parameter):
    if "intValue" in str(parameter):
        return str(parameter.intValue)
    elif "doubleValue" in str(parameter):
        return str(parameter.doubleValue)
    elif "boolValue" in str(parameter):
        return str(parameter.boolValue)
    elif "longValue" in str(parameter):
        return str(parameter.longValue)
    elif "stringValue" in str(parameter):
        return str(parameter.stringValue)


def _pipeline_component_info(model, info):
    """

    Parameters
    ----------
    model : pipeline model
    info : info dict to dump model related info into

    model can be one of 'arrayFeatureExtractor', 'categoricalMapping',
    'dictVectorizer', 'featureVectorizer', 'glmClassifier', 'glmRegressor',
    'identity', 'imputer', 'neuralNetwork', 'neuralNetworkClassifier',
    'neuralNetworkRegressor', 'normalizer', 'oneHotEncoder', 'scaler',
    'supportVectorClassifier', 'supportVectorRegressor',
    'treeEnsembleClassifier', 'treeEnsembleRegressor'

    Returns
    -------
    info : info dict with required info for model

    """
    model_type = model.WhichOneof("Type")
    if model_type == "arrayFeatureExtractor":
        info["desc"] = (
            "Given an index, extracts the value at "
            "that index from its array input.\n"
            "Indexes are zero-based."
        )
    elif model_type == "categoricalMapping":
        info["mappingType"] = _json.dumps(
            str(model.categoricalMapping.WhichOneof("MappingType"))
        )
        info["valueOnUnknown"] = _json.dumps(
            str(model.categoricalMapping.WhichOneof("ValueOnUnknown"))
        )
        info["desc"] = (
            "This allows conversion from integers "
            "to strings, or from strings to integers."
        )
    elif model_type == "dictVectorizer":
        info["map"] = _json.dumps(str(model.dictVectorizer.WhichOneof("Map")))
        info["desc"] = (
            "Uses an index mapping to convert a dictionary "
            "to an array.\n The output array will be equal in "
            "length to the index mapping vector parameter.\n"
            "All keys in the input dictionary must be present in "
            "the index mapping vector."
        )
    elif model_type == "featureVectorizer":
        info["inputList"] = _json.dumps(str(model.featureVectorizer.inputList))
        info["desc"] = (
            "A FeatureVectorizer puts one or more features into a single"
            " array.\n The ordering of features in the output array is "
            "determined by inputList."
        )
    elif model_type == "glmClassifier":
        info["offset"] = _json.dumps(str(model.glmClassifier.offset))
        info["postEvaluationTransform"] = _json.dumps(
            str(model.glmClassifier.postEvaluationTransform)
        )
        info["classEncoding"] = _json.dumps(str(model.glmClassifier.classEncoding))
        info["classLabels"] = _json.dumps(
            str(model.glmClassifier.WhichOneof("ClassLabels"))
        )
        info["desc"] = "A generalized linear model classifier."
    elif model_type == "glmRegressor":
        info["offset"] = _json.dumps(str(model.glmRegressor.offset))
        info["postEvaluationTransform"] = _json.dumps(
            str(model.glmRegressor.postEvaluationTransform)
        )
        info["desc"] = "A generalized linear model regressor."
    elif model_type == "imputer":
        info["ImputedValue"] = _json.dumps(
            str(model.imputer.WhichOneof("ImputedValue"))
        )
        info["desc"] = (
            "A transformer that replaces missing values with a "
            "default value,\n such as a statistically-derived "
            "value.\nIf ``ReplaceValue`` is set, then missing "
            "values of that type are\n replaced with the "
            "corresponding value."
        )
    elif model_type == "normalizer":
        info["normType"] = _json.dumps(str(model.normalizer.normType))
        info["desc"] = (
            "A normalization preprocessor.There are three normalization modes\n"
            "1. Max\n"
            "2. L1\n"
            "3. L2"
        )
    elif model_type == "oneHotEncoder":
        info["CategoryType"] = _json.dumps(
            str(model.oneHotEncoder.WhichOneof("CategoryType"))
        )
        info["outputSparse"] = _json.dumps(str(model.oneHotEncoder.outputSparse))
        info["handleUnknown"] = _json.dumps(str(model.oneHotEncoder.handleUnknown))
        info["desc"] = (
            "Transforms a categorical feature into an array. The array will be all\n"
            "zeros expect a single entry of one.\n"
            "Each categorical value will map to an index, this mapping is given by\n"
            "either the ``stringCategories`` parameter or the ``int64Categories``\n"
            "parameter."
        )
    elif model_type == "scaler":
        info["shiftValue"] = _json.dumps(str(model.scaler.shiftValue))
        info["scaleValue"] = _json.dumps(str(model.scaler.scaleValue))
        info["desc"] = (
            "A scaling operation.\n" r"f(x) = scaleValue \cdot (x + shiftValue)"
        )
    elif model_type == "supportVectorClassifier":
        info["kernel"] = _json.dumps(str(model.supportVectorClassifier.kernel))
        info["numberOfSupportVectorsPerClass"] = _json.dumps(
            str(model.supportVectorClassifier.numberOfSupportVectorsPerClass)
        )
        info["rho"] = _json.dumps(str(model.supportVectorClassifier.rho))
        info["probA"] = _json.dumps(str(model.supportVectorClassifier.probA))
        info["probB"] = _json.dumps(str(model.supportVectorClassifier.probB))
        info["ClassLabels"] = _json.dumps(
            str(model.supportVectorClassifier.WhichOneof("ClassLabels"))
        )
        info["desc"] = (
            "Support Vector Machine Classifier with one of "
            "Linear, RBF, Polynomial or Sigmoid "
            "kernels available"
        )
    elif model_type == "supportVectorRegressor":
        info["kernel"] = _json.dumps(str(model.supportVectorRegressor.kernel))
        info["numberOfSupportVectorsPerClass"] = _json.dumps(
            str(model.supportVectorRegressor.numberOfSupportVectorsPerClass)
        )
        info["rho"] = _json.dumps(str(model.supportVectorRegressor.rho))
        info["desc"] = (
            "Support Vector Machine Regressor with one of "
            "Linear, RBF, Polynomial or Sigmoid kernels available"
        )
    elif model_type == "treeEnsembleClassifier":
        info["treeEnsemble"] = _json.dumps(
            str(model.treeEnsembleClassifier.treeEnsemble)
        )
        info["postEvaluationTransform"] = _json.dumps(
            str(model.treeEnsembleClassifier.postEvaluationTransform)
        )
        info["ClassLabels"] = _json.dumps(
            str(model.treeEnsembleClassifier.WhichOneof("ClassLabels"))
        )
        info["desc"] = (
            "Each tree is a collection of nodes, each of which is identified "
            "by a unique identifier.\nEach node is either a branch or a leaf node."
            " A branch node evaluates a value according to a behavior;\n"
            "A tree must have exactly one root node, which has no parent node."
        )
    elif model_type == "treeEnsembleRegressor":
        info["treeEnsemble"] = _json.dumps(
            str(model.treeEnsembleRegressor.treeEnsemble)
        )
        info["postEvaluationTransform"] = _json.dumps(
            str(model.treeEnsembleRegressor.postEvaluationTransform)
        )
        info["desc"] = (
            "Each tree is a collection of nodes, each of which is identified"
            " by a unique identifier.\nEach node is either a branch or a leaf"
            " node. A branch node evaluates a value according to a behavior;\n"
            "A tree must have exactly one root node, which has no parent node."
        )
    return info


def _neural_network_node_info(nn_spec, cy_nodes, child=False, parent=None):
    """

    Parameters
    ----------
    nn_spec : Neural Network spec of mlmodel
    cy_nodes: list of nodes to update with nn layers
    child: If child of a parent pipeline component
    parent : Parent node of the Neural Network spec

    Returns
    -------

    cy_nodes: Updated with layer specific information

    """
    layers = nn_spec.layers
    for layer in layers:
        info = _layer_specific_info(layer)
        if child:
            info["name"] = layer.name
            cy_nodes.append(
                {
                    "data": {
                        "id": layer.name,
                        "name": info["type"],
                        "info": info,
                        "parent": parent,
                    },
                    "classes": info["type"],
                }
            )
        else:
            info["name"] = layer.name
            cy_nodes.append(
                {
                    "data": {"id": layer.name, "name": info["type"], "info": info},
                    "classes": info["type"],
                }
            )

    return cy_nodes


def _neural_network_nodes_and_edges(
    nn_spec, cy_nodes, cy_edges, spec_outputs, input_spec, input_shape_dict=None
):
    """

    Parameters
    ----------
    nn_spec : Neural Network Spec
    cy_nodes : list to add nn nodes to
    cy_edges : list to add edges for nn nodes to
    spec_outputs : outputs of nn spec
    input_spec : input spec of Neural Network

    Returns
    -------

    cy_data : concatenated list of updated cy_nodes and cy_edges

    """
    cy_nodes = _neural_network_node_info(nn_spec, cy_nodes)
    cy_nodes.append(
        {
            "data": {
                "id": "output_node",
                "name": "",
                "info": {"type": "output node"},
                "classes": "output",
            }
        }
    )

    for model_output, output_type in spec_outputs:
        cy_nodes.append(
            {
                "data": {
                    "id": str(model_output),
                    "name": str(model_output),
                    "info": {
                        "type": "\n".join(str(output_type).split("\n")),
                        "inputs": str([model_output]),
                        "outputs": str([]),
                    },
                    "parent": "output_node",
                },
                "classes": "output",
            }
        )

    shape_dict = _infer_shapes(nn_spec, input_spec, input_shape_dict=input_shape_dict)
    cy_nodes, cy_edges = _calculate_edges(cy_nodes, cy_edges, shape_dict)

    cy_data = cy_nodes + cy_edges
    return cy_data


def _pipeline_nodes_and_edges(cy_nodes, cy_edges, pipeline_spec, spec_outputs):
    """

    Parameters
    ----------
    cy_nodes : list to add nn nodes to
    cy_edges : list to add edges for nn nodes to
    pipeline_spec: Spec of pipeline mlmodel
    spec_outputs: spec outputs of pipeline mlmodel

    Returns
    -------

    cy_data : concatenated list of updated cy_nodes and cy_edges

    """
    i = 1
    nn_model_types = [
        "neuralNetwork",
        "neuralNetworkClassifier",
        "neuralNetworkRegressor",
    ]
    models = pipeline_spec.models
    shape_dict = None
    for model in models:
        sub_model_type = model.WhichOneof("Type")
        if not sub_model_type:
            sub_model_type = "input"
        info = {}
        input_names = []
        output_names = []
        info["Pipeline Component"] = sub_model_type.upper()
        for model_input in model.description.input:
            input_names.append(model_input.name)
            info["inputs"] = str(input_names)

        for model_output in model.description.output:
            output_names.append(model_output.name)
            info["outputs"] = str(output_names)

        info = _pipeline_component_info(model, info)

        if sub_model_type in nn_model_types:
            cy_nodes.append(
                {
                    "data": {
                        "id": "{}_{}".format(sub_model_type, i),
                        "name": sub_model_type,
                        "info": info,
                    },
                    "classes": "parent",
                }
            )
            if sub_model_type == "neuralNetwork":
                nn_spec = model.neuralNetwork
            elif sub_model_type == "neuralNetworkClassifier":
                nn_spec = model.neuralNetworkClassifier
            elif sub_model_type == "neuralNetworkRegressor":
                nn_spec = model.neuralNetworkRegressor
            cy_nodes = _neural_network_node_info(
                nn_spec, cy_nodes, child=True, parent="{}_{}".format(sub_model_type, i)
            )
            shape_dict = _infer_shapes(nn_spec, model.description.input)
        else:
            cy_nodes.append(
                {
                    "data": {
                        "id": "{}_{}".format(sub_model_type, i),
                        "name": sub_model_type,
                        "info": info,
                    },
                    "classes": sub_model_type,
                }
            )
        i += 1

    cy_nodes.append(
        {
            "data": {
                "id": "output_node",
                "name": "",
                "info": {"type": "output node"},
                "classes": "output",
            }
        }
    )

    for model_output, output_type in spec_outputs:
        cy_nodes.append(
            {
                "data": {
                    "id": str(model_output),
                    "name": str(model_output),
                    "info": {
                        "type": "\n".join(str(output_type).split("\n")),
                        "inputs": str([model_output]),
                        "outputs": str([]),
                    },
                    "parent": "output_node",
                },
                "classes": "output",
            }
        )

    cy_nodes, cy_edges = _calculate_edges(cy_nodes, cy_edges, shape_dict)

    cy_data = cy_nodes + cy_edges
    return cy_data


def _start_server(port, web_dir):
    """

    Parameters
    ----------
    port : localhost port to start server on
    web_dir: directory containing server files

    Returns
    -------

    None

    """
    curr_dir = _os.path.abspath(_os.curdir)
    _os.chdir(web_dir)
    import subprocess
    import sys
    import webbrowser

    if port is None:
        port = _np.random.randint(8000, 9000)
    if sys.version_info[0] <= 2:
        subprocess.Popen(["python", "-m", "SimpleHTTPServer", str(port)])
    else:
        subprocess.Popen(["python", "-m", "http.server", str(port)])
    webbrowser.open_new_tab("http://localhost:{}".format(str(port)))
    _os.chdir(curr_dir)
    return True
